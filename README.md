# iDASH 2022 Solution: Task 2
This is the solution of task 2 of G2Lab for iDASH competition in 2022.

## How to run

### SEAL library
We use SEAL library for HE scheme. In this docker, SEAL is already compiled, so you don't have to re-run it. However, if you find any trouble with it, then please follow the guide in the README file of SEAL to compile it again

### Build
Run CMake file to make binary file
```bash
    mkdir bin
    cd bin
    cmake ..
    make -j
```
As a result, you get two outputs: binary and continous.

### Run
`continuous` is for the models with continous phenotypes (1-3), and `binary` is for binary ones (4-5).
Run the following arguments.
```bash
    for pheno in 1 2 3;
        do ./continuous $pheno;
    done
    for pheno in 4 5;
        do ./binary $pheno;
    done
```
Otherwise, of course you can run each model one by one, or run one of following argument if you want to see the result of one model.
```bash
    ./continuous 1
    ./continuous 2
    ./continuous 3
    ./binary 4
    ./binary 5
```
They measure the time for each step and print at the end of the running. During process, ciphertexts are stored in the folder `savefiles/` and loaded to be used for the required step. The outputs of each model are stored in the folder `output/`.

## Accruacy
We provide python code for computing accruacies and combining outputs. Run following python code.
```bash
    cd ./
    python combine_result.py
```
As a result, the combined outputs are saved in the file `./all_models_final.csv`.