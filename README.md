# Cornell Compiler Project
Chenwei Hou (ch2352), Thomas Lu (cl2625), Steven Yu (ssy38)

## Project Info
Our compiler is written using C++23. We use CMake as our build system.

All of our source code is under `src/`. The source code for third-party libraries is under `libs/`.

### Using Docker
We use the beta version of the provided VM. To pull the Docker image, run:
```shell
docker pull sileiren/cs4120-env-x86:v4
```
To run the Docker image, run:
```shell
docker run -it -v <absolute path to "shared">:/home/student/shared sileiren/cs4120-env-x86:v4
```

### Using ANTLR4
We use ANTLR4 as our lexer/parser generator. To generate the lexer and parser code, we need to run `tools/antlr.jar` on the `src/etaLex.g4` and `src/etaParse.g4` files in `src/`:
```shell
java -jar ../tools/antlr.jar -Dlanguage=Cpp -o generated etaLex.g4 -no-listener -no-visitor
java -jar ../tools/antlr.jar -Dlanguage=Cpp -o generated etaParse.g4 -visitor
```
After this command is run, the generated lexer/parser code will be found in `src/generated/`.

## Project Structure
- `include/` - header files
- `libs/` - external libraries
- `src/` - source code
- `tests/` - tests
- `tools/` - tools (i.e. ANTLR4 jar, IR Interpreter jar)

## Building
To build the `etac` compiler, make sure you have CMake and a compiler that supports C++23 on your computer.

The following command generates the lexer/parser code, builds the compiler using CMake, and copies the `./etac` executable to the root directory of this project.
```shell
bash etac-build
```

## Running
To run the `etac` compiler, just run the `./etac` executable in the following form:
```shell
./etac [options] <source files>
```
*If you are using `--irrun`, you must run the `./etac` executable in the root of the repo to ensure the command to run the interpreter JAR is correct.* 

## Testing
To test our code, we use the provided `eth` test harness. We provide test cases and a test script (`ethScript`) in `tests/pa1/`, `tests/pa2/`, `tests/pa3/`, `tests/pa4/`, `tests/pa5/`, and `tests/pa6/`.

To run the tests for PA6, in the root of the repo, run the following command:
```shell
eth -runtimepath <runtime path> -compilerpath . -testpath ./tests/pa6 ./tests/pa6/ethScript
```

To run the tests for PA5, in the root of the repo, run the following command:
```shell
eth -runtimepath <runtime path> -compilerpath . -testpath ./tests/pa5 ./tests/pa5/ethScript
```

To run the tests for PA4, in the root of the repo, run the following command:
```shell
eth -compilerpath . -testpath ./tests/pa4 ./tests/pa4/ethScript
```

To run the tests for PA3, in the root of the repo, run the following command:
```shell
eth -compilerpath . -testpath ./tests/pa3 ./tests/pa3/ethScript
```

To run the tests for PA2, in the root of the repo, run the following command:
```shell
eth -compilerpath . -testpath ./tests/pa2 ./tests/pa2/ethScript
```

To run the tests for PA1, in the root of the repo, run the following command:
```shell
eth -compilerpath . -testpath ./tests/pa1 ./tests/pa1/ethScript
```
