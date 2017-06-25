import pandas as pd
import matplotlib.pyplot as plt
import pprint
import argparse

import subprocess
import os
import re
import multiprocessing
from multiprocessing import Pool
from collections import OrderedDict
import sys
import time

code_base_dir = None
output_dir = None
cmake_dir = None

benchmark_names = ["unique_ptr", "integer_class", "vector", "map", "shared_ptr", "polymorphic"]

mean_time_col = "Czas [ns]"
mean_cpu_col = "CPU [ns]"
mean_size_col = "Rozmiar [B]"
max_heap_col = "Max Sterta [B]"
alloc_count_col = "Liczba Alokacji"
archive_col = "Archiwum"
prob_size_col = "Wielkość"


def getXYLabels(test):
    if any(ext in test for ext in ["Polymorphic", "SharedPtr", "UniquePtr"]):
        x = "Wysokość drzewa"
    elif any(ext in test for ext in ["IntegerClassVect", "MapInt", "VectorFloat", "VectorInt"]):
        x = "Liczba elementów"
    else:
        x = None

    if "_Z_" in test:
        if "ns" in test:
            y = "Czas zapisu [ns]"
        else:
            y = "Rozmiar zapisanych danych [B]"
    elif "_O_" in test:
        y = "Czas odczytu [ns]"
    else:
        y = "Czas [ns]"

    return x, y


def archive_mapping(archive):
    if "c::Binary" in archive:
        return "Cereal Binary"
    elif "c::Extend" in archive:
        return "Cereal Extendable"
    elif "boost" in archive:
        if "no_header" in archive:
            return "Boost Binary"
        else:
            return "Boost Binary Header"
    elif "Proto" in archive:
        return "Protocol Buffers"
    else:
        return "Unknown"


class BenchmarkSingle:
    def __init__(self, line):
        # print(line)
        self.line = line
        self.save = re.match(r"^(Save|Load)", line).group(1) == "Save"
        self.name = re.match(r"^(Save|Load)(\w+)", line).group(2)
        self.archive = re.search(r"<(.*)>", line).group(1)
        has_size = re.search(r"/([0-9]+)", line)
        self.has_size = has_size is not None
        if self.has_size:
            self.size = has_size.group(1)
        else:
            self.size = "0"
        has_repeats = re.search(r"/repeats:([0-9]+)", line)
        self.has_repeats = has_repeats is not None
        if self.has_repeats:
            self.repeats = has_repeats.group(1)
        else:
            self.repeats = "0"

    def __repr__(self):
        return "Benchmark: %s : save? %r; name: %s; archive: %s; size:%s; repeats %s" % \
               (self.line, self.save, self.name, self.archive, self.size, self.repeats)


# name_regex = [r'\']
class Benchmark:
    def __init__(self, name_):
        self.name = name_
        self.getTests()

    def getBenchmarkDir(self, cmake_base):
        return os.path.join(cmake_base, "benchmark", self.name)

    def getBenchmarkExec(self):
        return "./benchmark_" + self.name

    def getTests(self):
        os.chdir(self.getBenchmarkDir(cmake_dir))
        output = subprocess.run([self.getBenchmarkExec(), "--benchmark_list_tests"],
                                stdout=subprocess.PIPE, universal_newlines=True)
        self.tests = []
        # print(output.stdout)
        for line in output.stdout.split("\n"):
            if re.match(r"(Save|Load)", line) is None:
                continue
            self.tests.append(BenchmarkSingle(line))

    def __repr__(self):
        return "Benchmark: %s\n %s" % (self.name, ["- %s\n" % x.__repr__() for x in self.tests])




def slugify(filename):
    return re.sub(r'[^a-zA-Z_0-9]+', '-', filename)


def run_cmake(cmake_base, test_heap):
    if not os.path.exists(cmake_base):
        os.makedirs(cmake_base)
    os.chdir(cmake_base)
    default_cmake = ["cmake", "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_BENCHMARKS=ON", code_base_dir]
    if test_heap:
        default_cmake.append("-DBENCHMARK_SINGLE_ITERATION=1")
    print(default_cmake)
    subprocess.call(default_cmake)


def run_make(cmake_base, target):
    os.chdir(cmake_base)
    subprocess.call(["make", "-j6", "VERBOSE=1", target])


benchmarks = []


def generate_benchmarks():
    for name in benchmark_names:
        benchmarks.append(Benchmark(name))
    pprint.pprint(benchmarks)


def get_max(category, name, program, arg):
    output = subprocess.run(["memusage", program, arg],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
    heap_peak = ""
    malloc_tot = ""
    realloc_tot = ""
    calloc_tot = ""
    for line in output.stdout.split('\n'):
        heap_peak_g = re.search(r"heap peak:\W+([0-9]+),", line)
        if heap_peak_g:
            heap_peak = heap_peak_g.group(1)
            continue

        malloc_tot_g = re.search(r"malloc\|[^ ]*\W+([0-9]+)", line)
        if malloc_tot_g:
            malloc_tot = malloc_tot_g.group(1)
            continue

        realloc_tot_g = re.search(r"realloc\|[^ ]*\W+([0-9]+)", line)
        if realloc_tot_g:
            realloc_tot = realloc_tot_g.group(1)
            continue

        calloc_tot_g = re.search(r"calloc\|[^ ]*\W+([0-9]+)", line)
        if calloc_tot_g:
            calloc_tot = calloc_tot_g.group(1)
            break
    alloc_tot = int(malloc_tot) + int(realloc_tot) + int(calloc_tot)
    return category, name, heap_peak, malloc_tot, realloc_tot, calloc_tot, alloc_tot


def run_in_background(fun, tasks, threads):
    waiting = []
    results = []
    # processes=multiprocessing.cpu_count() / 2
    with Pool(processes=threads) as pool:
        for task in tasks:
            waiting.append(
                pool.apply_async(fun,
                                 task))
        print("waiting")
        for result in waiting:
            results.append(result.get())
            print("processed: %s" % len(results))

    return results


def test_heap(cmake_base, output_file):
    tasks = []
    for benchmark in benchmarks:
        program_name = benchmark.getBenchmarkExec()
        bdir = benchmark.getBenchmarkDir(cmake_base)
        program = os.path.join(bdir, program_name)
        for test in benchmark.tests:
            tasks.append((benchmark.name, test.line, program, "--benchmark_filter=%s" % test.line,))

    print("tasks: %s" % len(tasks))
    result = run_in_background(get_max, tasks, multiprocessing.cpu_count())
    df = pd.DataFrame(result, columns=["File", "Benchmark", "HeapPeak", "Malloc", "Realloc", "Calloc", "AllocCalls"])
    df.to_csv(os.path.join(output_dir, output_file + ".csv"), encoding='utf-8')
    df.to_pickle(os.path.join(output_dir, output_file + ".pkl"))
    print("saved to: %s" % os.path.join(output_dir, output_file + ".csv"))


def run_benchmark(category, name, program, filter, output, output_format):
    output = subprocess.run([program, filter, output, output_format],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
    return True


def test_all(cmake_base, output_dir):
    tasks = []
    timestr = time.strftime("%Y%m%d-%H%M%S")
    for benchmark in benchmarks:
        program_name = benchmark.getBenchmarkExec()
        bdir = benchmark.getBenchmarkDir(cmake_base)
        program = os.path.join(bdir, program_name)
        benchmarks_in_group = set()
        for test in benchmark.tests:
            filter = "%s<%s>" % (test.name, test.archive)
            filename = slugify(filter)
            benchmarks_in_group.add((filter, filename))

        for (filter, filename) in benchmarks_in_group:
            tasks.append((benchmark.name, test.line, program, "--benchmark_filter=%s" % filter,
                          "--benchmark_out=%s" % os.path.join(output_dir,
                                                              ("%s-%s-%s.csv" % (benchmark.name, filename, timestr))),
                          "--benchmark_out_format=csv"))

    print("tasks: %s" % len(tasks))
    result = run_in_background(run_benchmark, tasks, multiprocessing.cpu_count() // 2)


def strip_mean(row):
    return re.sub(r'_mean', '', row['name'])


def is_mean(row):
    return re.search(r'_mean$', row['name']) is not None


def attach_stddev(row, all, col_name):
    if re.search(r'_mean$', row['name']) is not None:
        searching = re.sub(r'_mean', '_stddev', row['name'])
        # print("searching %s" % searching)
        a = all[all['name'] == searching][col_name].iloc[0]
        return a


def format_float(num):
    if isinstance(num, float):
        if pd.isnull(num):
            return ""
        else:
            return "{0:.2f}".format(num)
    else:
        return num


def new_frame(row):
    b = BenchmarkSingle(row['name'])
    b_saved = row["bytesSaved"]
    return pd.Series(
        OrderedDict([
            ("Nazwa", b.name),
            ("Op", "Z" if b.save else "O"),
            (archive_col, archive_mapping(b.archive)),
            (prob_size_col, int(b.size)),
            ("Powtórzenia", int(b.repeats)),
            (mean_time_col, float(row["real_time"])),
            (mean_cpu_col, float(row["cpu_time"])),
            ("σ Czasu", float(row["StdDevReal"])),
            ("σ CPU", float(row["StdDevCpu"])),
            (mean_size_col, float(b_saved) if not pd.isnull(b_saved) else ""),
            (max_heap_col, int(row["HeapPeak"])),
            (alloc_count_col, int(row["AllocCalls"]))
        ])
    )


def strip_iterations_from_name(name):
    return re.sub(r'/iterations:[0-9]+', r'', name)


def read_bench_output(directory, heap_file, output):
    dfs = []
    for file in os.listdir(directory):
        full_filename = os.path.join(directory, file)
        df_tmp = pd.read_csv(full_filename, skiprows=[0, 1])
        dfs.append(df_tmp)
    out_df = pd.concat(dfs)
    # out_df.columns = out_df.columns.str.replace('name', 'Benchmark')
    benchmark_name = out_df.apply(lambda row: strip_mean(row), axis=1)
    out_df['Benchmark'] = benchmark_name
    out_df['HasMean'] = out_df.apply(lambda row: is_mean(row), axis=1)
    out_df['StdDevReal'] = out_df.apply(lambda row: attach_stddev(row, out_df, 'real_time'), axis=1)
    out_df['StdDevCpu'] = out_df.apply(lambda row: attach_stddev(row, out_df, 'cpu_time'), axis=1)

    # heap data manipulation
    heap_df = pd.read_csv(heap_file, index_col=0)
    heap_df['Benchmark'] = heap_df['Benchmark'].apply(strip_iterations_from_name)
    heap_df.to_csv(output + "_heap_dirty.csv")

    out_df = out_df.merge(heap_df, how='outer', on="Benchmark")

    # print(out_df)
    out_df.to_csv(output + "_all.csv")
    out_df.drop(out_df[out_df.name.str.contains('stddev')].index, inplace=True)
    out_df.to_csv(output + "_ok.csv")

    new_df = pd.DataFrame(out_df.apply(lambda row: new_frame(row), axis=1))
    # print(new_row)
    # new_df.append(new_row)
    new_df.sort_values(["Nazwa", "Op", prob_size_col, archive_col], ascending=[1, 0, 1, 1], inplace=True)
    # print(new_df)
    new_df.to_csv(output + "_new.csv")
    new_df.stack().to_csv(output + "_new_stacked.csv")

    return new_df


def add_baseline(row, df, check_size):
    # print("Base: %s" % base)
    # print("Row: %s" % row)
    cols = list()
    baseline = df[df['Archiwum'].isin(["Boost Binary"])]
    # print("Problem: %s" % row)
    # print("Problem base: %s" % baseline)
    if baseline.shape[0] != 1:  # more than one row
        baseline = baseline[baseline['Wielkość'] == row['Wielkość']]

    cols.append(("time",
                 float(row[mean_time_col] / baseline[mean_time_col])
                 ))
    cols.append(("heap",
                 row[max_heap_col] / baseline[max_heap_col].iloc[0]
                 ))
    cols.append(("alloc",
                 row[alloc_count_col] / baseline[alloc_count_col].iloc[0]
                 ))
    # if mean_size_col in row:
    if check_size:
        cols.append(("size",
                     float(row[mean_size_col] / baseline[mean_size_col])
                     ))
    # print("T: %s; H: %s; S: %s" % (time_c, size_c, heap_c))
    # print("End")

    return pd.Series(OrderedDict(cols))


def drop_n_unique(df):
    uniques = df.apply(lambda x: x.nunique())
    return df.drop(uniques[uniques == 1].index, axis=1)


def process_test(df, name, op, directory):
    # uniques = df.apply(lambda x: x.nunique())
    # df_out = df.drop(uniques[uniques == 1].index, axis=1)
    df_out = df
    has_size = df[mean_size_col].nunique() != 1
    cols = df_out.apply(lambda row: add_baseline(row, df_out, has_size), axis=1)
    cols = pd.DataFrame(cols)
    df_out.insert(df_out.columns.get_loc(mean_time_col) + 1, "Czas X", cols["time"])
    df_out.insert(df_out.columns.get_loc(max_heap_col) + 1, "Sterta X", cols["heap"])
    df_out.insert(df_out.columns.get_loc(alloc_count_col) + 1, "Allokacje X", cols["alloc"])
    if cols.shape[1] == 4:
        df_out.insert(df_out.columns.get_loc(mean_size_col) + 1, "Rozmiar X", cols["size"])

    drop_n_unique(df_out).to_csv(os.path.join(directory, name + op + "_new.csv"), float_format="%0.2f")
    # df.merge(df_out)
    return df_out


def set_xy_labels(filename):
    x, y = getXYLabels(filename)
    if x is not None:
        plt.xlabel(x)
    if y:
        plt.ylabel(y)


def gen_plots(df, directory):
    plt.style.use('ggplot')
    g_name = df.groupby(["Nazwa"])
    for name in g_name:
        g_op = name[1].groupby(["Op"])
        for op in g_op:
            tmp_df = drop_n_unique(op[1])
            for col in [mean_time_col, mean_size_col]:
                if col not in tmp_df:
                    continue
                filename = "%s_%s_%s_plot.pdf" % (name[0], op[0], slugify(col))
                set_xy_labels(filename)
                plt.figure(figsize=(6, 3))
                if prob_size_col in tmp_df:
                    # fig = plt.figure()
                    for archive, rest in tmp_df.groupby([archive_col]):
                        plt.plot(rest[prob_size_col], rest[col], '-+', label=archive)
                        if (rest[prob_size_col] > 100).any():
                            plt.loglog(basex=2, basey=10)
                        else:
                            plt.xscale('linear')
                            plt.yscale('log', basey=10)

                        plt.xticks(rest[prob_size_col])
                        plt.legend(loc='best')

                else:
                    tmp_df.plot(x=archive_col, y=col, kind='bar', rot=12, alpha=0.75)
                    plt.legend().remove()

                # plt.aspect('auto')
                # plt.axes().set_aspect('211')
                set_xy_labels(filename)
                plt.tight_layout()
                # plt.legend(loc='best')
                # plt.show()
                plt.savefig(os.path.join(directory, filename))
                plt.close('all')


def gen_plots2(df, directory):
    plt.style.use('ggplot')
    g_name = df.groupby(["Nazwa"])
    for name in g_name:
        tmp_df = drop_n_unique(name[1])
        if prob_size_col in tmp_df:
            continue
        filename = "%s_%s_plot_joined.pdf" % (name[0], slugify(mean_time_col))
        left = tmp_df[tmp_df["Op"] == "Z"][[archive_col, mean_time_col]]
        left.columns = [archive_col, "Zapis"]
        right = tmp_df[tmp_df["Op"] == "O"][[archive_col, mean_time_col]]
        right.columns = [archive_col, "Odczyt"]
        tmp_df = pd.merge(left, right)
        tmp_df.set_index([archive_col], inplace=True)
        set_xy_labels(filename)
        plt.figure(figsize=(5, 3))
        tmp_df.plot(kind='bar', rot=12, alpha=0.75)
        # plt.show()
        set_xy_labels(filename)
        plt.legend(loc='best')
        plt.tight_layout()
        plt.savefig(os.path.join(directory, filename))
        plt.close('all')


def change_columns_latex(columns):
    out = []
    for column in columns:
        if "σ" in column:
            out.append(column.replace("σ", "$\sigma$"))
        elif " X" in column:
            out.append("$\div$")
        elif "Sterta" in column:
            out.append("Sterta")
        elif "Rozmiar" in column:
            out.append("R")
        elif "Czas [ns]" in column:
            out.append("Czas")
        elif alloc_count_col in column:
            out.append("Alok.")
        elif "Wielkość" in column:
            out.append("N")
        elif "Archiwum" in column:
            out.append("A")
        else:
            out.append(column)
    return out


def change_second_column_for_split(df, parbox):
    if parbox:
        df["Archiwum"] = df["Archiwum"].apply(lambda x:
                                              re.sub(r'(^.*$)', r'\\parbox{1.5cm}{\1}',
                                                     re.sub(r'(\w+)', r'\\makebox{\1}', x)
                                                     )[0])
    else:
        df["Archiwum"] = df["Archiwum"].apply(lambda x: re.sub(r'(\w+)', r'\\makebox{\1}', x))
    return df

def change_second_column_for_split(df, parbox):
    if parbox:
        # with multirow we don't need p{1.5cm}
        df["Archiwum"] = df["Archiwum"].apply(lambda x:
                                              re.sub(r'(^.*$)', r'\\parbox{1.5cm}{\1}',
                                                     re.sub(r'(\w+)', r'\\makebox{\1}', x)
                                                     )[0])
    else:
        df["Archiwum"] = df["Archiwum"].apply(lambda x: re.sub(r'(\w+)', r'\\makebox{\1}', x))
    return df


def shorten_data_in_columns(df):
    for column in df.columns:
        if column == "Archiwum":
            df[column] = df[column].apply(lambda x: re.sub(r'(\w)\w+\W*', r'\1', x))
        elif column == mean_time_col or column == mean_cpu_col:
            df[column] = df[column].apply(lambda x: int(round(x)))
        elif column == mean_size_col:
            df[column] = df[column].apply(lambda x: int(round(x)) if isinstance(x, float) and not pd.isnull(x) else x)
        elif column == max_heap_col:
            df[column] = df[column].apply(lambda x: str(int(round(x / 1000))) + "k" if x >= 10 ** 3 else x)
        elif column == alloc_count_col:
            df[column] = df[column].apply(lambda x: format_float(x/1000) + "k" if x >= 10**3 else x)

    return df


def move_columns_to_end(df, columns):
    o_cols = df.columns.tolist()
    for column in columns:
        pos = len(o_cols) - 1
        o_cols.insert(pos, o_cols.pop(o_cols.index(column)))

    return df.reindex(columns=o_cols)


def latex_drop_columns(df, columns):
    o_cols = df.columns.tolist()
    for column in columns:
        o_cols.pop(o_cols.index(column))

    return df.reindex(columns=o_cols)


def latex_drop_rows(df):
    if prob_size_col not in df:
        return df
    uniques = df[prob_size_col].unique()
    print("Uniques %s" % uniques)
    take = [uniques[0],
            uniques[len(uniques) // 2],
            uniques[len(uniques) - 1]
            ]
    if len(uniques) >= 10:
        take.insert(1, uniques[1])
    df = df[df[prob_size_col].isin(take)]
    return df


def gen_latex(df, directory):
    g_name = df.groupby(["Nazwa"])
    for name in g_name:
        tmp_df = drop_n_unique(name[1])
        filename = os.path.join(directory, name[0] + "_table.tex")
        # tmp_df = move_columns_to_end(tmp_df, ["CPU [ns]", "σ CPU", "σ Czasu"])
        tmp_df = latex_drop_columns(tmp_df, ["CPU [ns]", "σ CPU", "σ Czasu"])
        sort_cols = (["Op", "Archiwum"], [0, 1])
        if "Wielkość" in tmp_df:
            sort_cols[0].append("Wielkość")
            sort_cols[1].append(1)
            tab_format = "lll|" + "rl" * ((tmp_df.shape[1] - 3) // 2)
        else:
            tab_format = "ll|" + "rl" * ((tmp_df.shape[1] - 2) // 2)  # "lp{1.6cm}" + "r" * (tmp_df.shape[1] - 2)
        tmp_df = tmp_df.sort_values(sort_cols[0], ascending=sort_cols[1])
        # tmp_df = change_second_column_for_split(tmp_df, "N" in tmp_df)
        tmp_df = shorten_data_in_columns(tmp_df)
        tmp_df = latex_drop_rows(tmp_df)
        tmp_df = tmp_df.set_index(sort_cols[0])
        # After this line name of columns is changed
        tmp_df.columns = change_columns_latex(tmp_df.columns)
        tmp_df.index.names = change_columns_latex(tmp_df.index.names)

        tmp_df.to_latex(filename, float_format=format_float,
                        index=True, escape=False, longtable=False,
                        sparsify=True, multirow=True, column_format=tab_format)


def generate_all(df, directory):
    f_dfs = pd.DataFrame()
    max_col = []
    g_name = df.groupby(["Nazwa"])
    for name in g_name:
        g_op = name[1].groupby(["Op"])
        for op in g_op:
            out_df = process_test(op[1], name[0], op[0], directory)
            # with concat order is not preserved
            if len(max_col) < len(out_df.columns.tolist()):
                max_col = out_df.columns.tolist()
            f_dfs = pd.concat([out_df, f_dfs])

    f_dfs = f_dfs[max_col]
    f_dfs.to_csv(os.path.join(directory, "all_new_2.csv"), float_format="%0.2f")

    # now plots
    # gen_plots(f_dfs, directory)
    # gen_plots2(f_dfs, directory)
    gen_latex(f_dfs, directory)

def archive_cmake_size_mapping(archive):
    if "cereal_binary" in archive:
        return "Cereal Binary"
    elif "cereal_extendable" in archive:
        return "Cereal Extendable"
    elif "boost" in archive:
            return "Boost Binary"
    elif "protobuf" in archive:
        return "Protocol Buffers"
    else:
        print("Unknown is %s" % archive)
        return "Unknown"

def test_cmake_size_mapping(test):
    if "integer_class" in test:
        return "IntegerClass, IntegerClassVect"
    elif "map" in test:
        return "Mapy"
    elif "polymorphic" in test:
        return "Wsk. polimorficzne"
    elif "shared_ptr" in test:
        return "Wsk. współdzielone"
    elif "vector" in test:
        return "Tablice"
    elif "unique_ptr" in test:
        return "Wsk. unikalne"
    else:
        return "Unknown"


def read_exe_sizes(dir):
    rows = []
    size_dir = os.path.join(dir, "size")
    for file in os.listdir(size_dir):
        if not "size_benchmark" in file:
            continue
        name_split_g = re.search(r'size_benchmark_([a-z_]+)-([a-z_]+)', file)
        row = [
            ("Zbiór testów", test_cmake_size_mapping(name_split_g[1])),
            ("Biblioteka", archive_cmake_size_mapping(name_split_g[2])),
            ("Rozmiar [KB]", os.path.getsize(os.path.join(size_dir, file))/1024)
            ]
        name_split = pd.Series(OrderedDict(row))
        # print(name_split)
        rows.append(name_split)
    df = pd.DataFrame(rows)
    df.sort_values(["Zbiór testów", "Biblioteka"], inplace=True)
    df.set_index(["Zbiór testów", "Biblioteka"], inplace=True)
    return df



def generate_size_table(cmake_dir_, output_dir_):
    df = read_exe_sizes(cmake_dir_)
    print(df)
    df.to_latex(os.path.join(output_dir_, "sizes.tex"), float_format=format_float,
                    index=True, escape=False, longtable=False,
                    sparsify=True, multirow=True)  # output_file_base


parser = argparse.ArgumentParser()
parser.add_argument("--output_dir", help="output directory", type=str, required=True)
parser.add_argument("--data_dir", help="directory with data; output dir from last run with --heap and --bench",
                    type=str, required=True)
parser.add_argument("-c", "--cmake_dir", help="cmake directory; by default output_dir/cmake", type=str)
parser.add_argument("--code_dir", help="base directory for cereal", type=str)
parser.add_argument("--heap", help="generate heap profile; in output directory heap.csv will be created",
                    action="store_true")
parser.add_argument("-b", "--bench", help="generate benchmark output", action="store_true")
parser.add_argument("--gen_sizes", help="generate size data", action="store_true")

parser.add_argument("-r", "--read", help="read benchmark output", action="store_true")
parser.add_argument("--skip_build", help="skip running cmake and make", action="store_true", default=False)

if __name__ == '__main__':
    args = parser.parse_args()
    output_dir = args.output_dir
    data_dir = args.data_dir

    if args.cmake_dir:
        cmake_dir = args.cmake_dir
    else:
        cmake_dir = os.path.join(output_dir, "cmake")

    if not args.skip_build:
        if not args.code_dir:
            print("code_dir is needed. Use --skip_build or provide directory to source code")
            sys.exit(1)
        if not os.path.exists(args.code_dir):
            print("code_dir has to be valid path: %s" % args.code_dir)
            sys.exit(1)
        code_base_dir = args.code_dir
        run_cmake(cmake_dir, args.heap)
        run_make(cmake_dir, "benchmarks")
        generate_benchmarks()
    if args.gen_sizes:
        if args.skip_build:
            print("calling skip build and gen_sizes is not supported")
            sys.exit(1)
        run_make(cmake_dir, "size_benchmark")
        generate_size_table(cmake_dir, os.path.join(output_dir, "output"))
    if args.heap:
        test_heap(cmake_dir, "heap")

    if args.bench:
        directory = os.path.join(output_dir, "bench")
        if not os.path.exists(directory):
            os.makedirs(directory)
        test_all(cmake_dir, directory)

    if args.read:
        dir_out = os.path.join(output_dir, "output")
        if not os.path.exists(dir_out):
            os.makedirs(dir_out)

        df = read_bench_output(os.path.join(data_dir, "bench"), os.path.join(data_dir, "heap.csv"),
                               os.path.join(output_dir, "merged"))

        generate_all(df.copy(deep=True), dir_out)
