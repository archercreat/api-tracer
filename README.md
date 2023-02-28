# api-tracer - tiny (useless) tracer

api-tracer is a dynamic library that, when loaded into a process, intercepts and logs each exported function from a list of intercepted libraries. It was a fun project to test some functionality when use of DBI/Debugger is not possible.

## Usage

The project comes with the simple loader that creates suspended process and injects `trace.dll`:
```
> loader.exe --help
Usage: api-tracer: trace.dll Loader [--help] [--version] --exe VAR [--tracer VAR] args

Positional arguments:
  args          Arguments to pass to target executable [nargs: 0 or more] [default: {}]

Optional arguments:
  -h, --help    shows help message and exits
  -v, --version prints version information and exits
  -e, --exe     Target executable to trace [default: ""]
  -t, --tracer  Path to tracer.dll [default: ""]
```

In order to use api-tracer, one needs to place `hooks.txt` file that contains list of dlls seperated by new line in the current directory. To filter api calls from specific modules, place `filter.txt` in the current directory with the list of modules. The output will be logged in `trace.txt` file.
