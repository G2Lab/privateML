#include "seal/seal.h"
#include <iomanip>
#include <fstream>

using namespace std;
using namespace seal;

// CSV file to double vector
vector<double> CSVtoVector(string filename)
{
    vector<double> result_vector;
    // vector<string> string_vector;
    ifstream data(filename);
    string line;
    // int line_count = 0;
    // string cell;

    while (getline(data, line))
    {
        // string_vector.push_back(line);
        double weight = ::stof(line);
        result_vector.push_back(weight);
        // cout << weight << " ";
    }
    return result_vector;
}

// print contents of vector
template <typename T>
void print_vector(vector<T> printme)
{
    for (int j = 0; j < printme.size(); j++)
    {
        cout << printme[j] << " ";
    }
}

// TSV to string matrix converter
/// Must be modified to clean up unnecessary columns!!
vector<vector<int>> TSVtoMatrix(string filename)
{
    vector<vector<int>> result_matrix;

    ifstream data(filename);
    string line;
    int line_count = 0;
    while (getline(data, line))
    {
        stringstream lineStream(line);
        string cell;
        vector<int> parsedRow;
        int word_count = 0;
        while (getline(lineStream, cell, '\t'))
        {
            if (word_count > 3)
            {
                int entry = ::stof(cell);
                parsedRow.push_back(entry);
            }
            word_count++;
            // cout << entry << " ";
        }
        result_matrix.push_back(parsedRow);
    }
    return result_matrix;
}

double CSVtoConst(string filename)
{
    // vector<string> string_vector;
    ifstream data(filename);
    string line;
    // int line_count = 0;
    // string cell;
    double weight;
    while (getline(data, line))
    {
        // string_vector.push_back(line);
        weight = ::stof(line);
        // result_vector.push_back(weight);
        // cout << weight << ” “;
    }
    return weight;
}

// Matrix Transpose
template <typename T>
vector<vector<T>> transpose_matrix(vector<vector<T>> input_matrix)
{

    int rowSize = input_matrix.size();
    int colSize = input_matrix[0].size();
    vector<vector<T>> transposed(colSize, vector<T>(rowSize));

    for (int i = 0; i < rowSize; i++)
    {
        for (int j = 0; j < colSize; j++)
        {
            transposed[j][i] = input_matrix[i][j];
        }
    }

    return transposed;
}

// printing out a vector of vectors
template <typename T>
void print_nested_vec(vector<vector<T>> matrix)
{
    // vector<vector<double>> result(matrix.size(), vector<double>(matrix[0].size()));
    for (int i = 0; i < matrix.size(); i++)
    {
        cout << "\n<<<" << i << "row:>>>" << endl;
        for (int j = 0; j < matrix[0].size(); j++)
        {
            cout << matrix[i][j] << " ";
        }
    }
}

vector<int64_t> ScaleVector(vector<double> &v, int k)
{
    vector<int64_t> intvec(v.size(), 0);
    for (int i = 0; i < v.size(); ++i)
        intvec[i] = v[i] * k;
    return intvec;
}

void SaveGenotypeEnc(vector<vector<Ciphertext>> genotype, string filename)
{
    // cout << "prepare save & load" << endl;
    ofstream ofs;
    ifstream ifs;

    cout << "start saving genotype encryptions" << endl;
    stringstream data_stream;
    for (int i = 0; i < genotype.size(); i++)
    {
        for (int j = 0; j < genotype[i].size(); j++)
        {
            auto size_encrypted1 = genotype[i][j].save(data_stream);
        }
    }
    ofs.open(filename, ios::binary);
    ofs << data_stream.rdbuf();
    ofs.close();
    cout << "saving done" << endl;
}

void SaveVec(vector<Ciphertext> weights, string filename)
{
    // cout << "prepare save & load" << endl;
    ofstream ofs;
    ifstream ifs;

    cout << "start saving ciphertext vector" << endl;
    stringstream data_stream;
    for (int i = 0; i < weights.size(); i++)
    {
        auto size_encrypted1 = weights[i].save(data_stream);
    }

    ofs.open(filename, ios::binary);
    ofs << data_stream.rdbuf();
    ofs.close();
    cout << "saving done" << endl;
}

void SaveResult(vector<double> final_result, string filename)
{
    ofstream ofs;

    cout << "start saving final result vector" << endl;
    stringstream data_stream;

    for (int i = 0; i < final_result.size(); i++)
    {
        // double result = final_result[i].save(data_stream);
        data_stream << final_result[i];
        if (i < final_result.size() - 1) data_stream << endl;
    }

    ofs.open(filename);
    ofs << data_stream.rdbuf();
    ofs.close();
    cout << "saving done" << endl;
}

vector<vector<Ciphertext>> LoadGenotype(string filename, SEALContext context, int num_samples, int size)
{
    cout << "Encrypted genotype loading" << endl;
    ofstream ofs;
    ifstream ifs;
    stringstream data_stream_load;
    ifs.open(filename, ios::binary);
    data_stream_load << ifs.rdbuf();
    ifs.close();
    vector<vector<Ciphertext>> ctxt_load(num_samples, vector<Ciphertext>(size));
    for (int i = 0; i < num_samples; i++)
    {
        for (int j = 0; j < size; j++)
        {
            ctxt_load[i][j].load(context, data_stream_load);
        }
    }
    cout << "loaded done" << endl;
    return ctxt_load;
}

vector<Ciphertext> LoadVec(string filename, SEALContext context, int size)
{
    cout << "Encrypted Model loading" << endl;
    ofstream ofs;
    ifstream ifs;
    stringstream data_stream_load;
    ifs.open(filename, ios::binary);
    data_stream_load << ifs.rdbuf();
    ifs.close();
    vector<Ciphertext> ctxt_load(size);
    for (int j = 0; j < size; j++)
    {
        ctxt_load[j].load(context, data_stream_load);
    }
    cout << "loaded done" << endl;
    return ctxt_load;
}
