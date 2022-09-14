#include "seal/seal.h"
#include <iostream>
#include "helper.h"
#include <functional>
#include <cmath>
#include "SEAL/native/examples/examples.h"
#include <sys/wait.h>
#include <unistd.h>

using namespace std;
using namespace seal;

int main(int argc, char *argv[])
{
    string phenotype = argv[1];

    EncryptionParameters parms(scheme_type::bfv);
    size_t poly_modulus_degree = 4096;
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {35, 30, 35}));

    parms.set_plain_modulus(PlainModulus::Batching(poly_modulus_degree, 25));
    SEALContext context(parms);
    print_parameters(context);
    auto &context_data = *context.key_context_data();
    uint64_t plain_modulus = context_data.parms().plain_modulus().value();
    KeyGenerator keygen(context);
    SecretKey secret_key = keygen.secret_key();
    PublicKey public_key;
    keygen.create_public_key(public_key);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    Encryptor encryptor_sec(context, secret_key);
    Encryptor encryptor_pub(context, public_key);
    Evaluator evaluator(context);
    Decryptor decryptor(context, secret_key);

    BatchEncoder batch_encoder(context);
    int slot_count = batch_encoder.slot_count();

    string geno = "../data/testset.tsv";
    string coef = "../weights/weight" + phenotype + ".csv";
    
    int num_samples;

    auto start_enc_geno = std::chrono::high_resolution_clock::now();
    string filename = "../savefiles/genotype_ctxt" + phenotype + ".dat";
    cout << "\n-----------ENCRYPTING GENOTYPE (Client) -----------\n"
         << endl;
    cout << "\nReading in the genotype:" << endl;
    vector<vector<int>> genotype = TSVtoMatrix(geno);
    vector<vector<int>> geno_T = transpose_matrix(genotype);
    cout << "\nEncoding genotype:" << endl;
    num_samples = geno_T.size();
    int num_full_ctxt = 20390 / poly_modulus_degree;
    vector<vector<Plaintext>> g(num_samples, vector<Plaintext>(num_full_ctxt + 1));
    for (int sample = 0; sample < num_samples; sample++)
    {
        // encode zero vector to plaintext: this is because we don't know how to make zero plain matrix yet :(
        // -----For genotype, insert in order
        vector<int64_t> vec1(slot_count);
        for (int j = 0; j < num_full_ctxt + 1; j++) {
            batch_encoder.encode(vec1, g[sample][j]);
            for (int i = 0; i < g[sample][0].coeff_count(); i++)
            {
                g[sample][j].data()[i] = 0;
            }
        }
        // for Plaintext, plain.data()[index] means the "index"th coefficient of plaintext
        // pad zero for all coefficients
        for (int j = 0; j < num_full_ctxt; j++) {
            for (int i = 0; i < g[sample][0].coeff_count(); i++)
            {
                g[sample][j].data()[i] = geno_T[sample][i + j * g[sample][0].coeff_count()];
            }
        }
        
        int remainder = geno_T[sample].size() - num_full_ctxt * g[sample][0].coeff_count();
        for (int i = 0; i < remainder; i++)
        {
            g[sample][num_full_ctxt].data()[i] = geno_T[sample][i + num_full_ctxt * g[sample][0].coeff_count()];
        }
    }

    cout << "\nEncrypting snp vectors..." << endl;
    vector<vector<Ciphertext>> gcipher(num_samples, vector<Ciphertext>(num_full_ctxt + 1));
    for (int sample = 0; sample < num_samples; sample++)
    {
        for (int j = 0; j < num_full_ctxt + 1; j++) {
            encryptor_sec.encrypt_symmetric(g[sample][j], gcipher[sample][j]);
        }
    }
    // SAVING ENCRYPTED GENOTYPE
    cout << "Enc encryption" << endl;
    cout << "save ctxt to " << filename << endl;
    SaveGenotypeEnc(gcipher, filename);
    auto end_enc_geno = std::chrono::high_resolution_clock::now();

    /*
    -----For weight, insert backwards after the first element
    If element is negative, insert positive element (-element) as coefficient
    If element is positive, insert plain_modulus - element as coefficient
    */
    int scaling = pow(2, 20);
    string modelname = "../savefiles/model_ctxt" + phenotype + ".dat";
    
    auto start_enc_model = std::chrono::high_resolution_clock::now();
    cout << "\n-----------ENCRYPTING MODEL (Modeler) -----------\n"
         << endl;
    
    cout << "\nReading in the weights input:" << endl;
    vector<double> input_weights = CSVtoVector(coef);
    vector<int64_t> weights = ScaleVector(input_weights, scaling);
    
    cout << "\nEncoding weights..." << endl;
    vector<int64_t> vec4(slot_count);
    vector<Plaintext> w(num_full_ctxt + 1); 
    for (int j = 0; j < w.size(); j++) {
        batch_encoder.encode(vec4, w[j]);
    }
    // padding zero for the coefficients initially
    for (int i = 0; i < w[0].coeff_count(); i++)
    {
        for (int j = 0; j < w.size(); j++) {
            w[j].data()[i] = 0;        
        }
    }
    
    int num_coeff = w[0].coeff_count();
    for (int k = 0; k < num_full_ctxt; k++) {
        for (int j = 0; j < w[k].coeff_count(); j++)
        {
            if (j == 0)
            {
                if (weights[j + k * num_coeff] < 0)
                {
                    w[k].data()[j] = plain_modulus - weights[j + k * num_coeff];
                }
                else
                {
                    w[k].data()[j] = weights[j + k * num_coeff];
                }
            }
            else
            {
                if (weights[j + k * num_coeff] > 0)
                {
                    w[k].data()[poly_modulus_degree - j] = plain_modulus - weights[j + k * num_coeff];
                }
                else
                {
                    w[k].data()[poly_modulus_degree - j] = -weights[j + k * num_coeff];
                }
            }
        }
    }
    
    int remainder = weights.size() - num_full_ctxt * num_coeff;
    for (int j = 0; j < remainder; j++)
    {
        if (j == 0)
        {
            if (weights[j + num_full_ctxt * num_coeff] < 0)
            {
                w[num_full_ctxt].data()[j] = plain_modulus - weights[j + num_full_ctxt * num_coeff];
            }
            else
            {
                w[num_full_ctxt].data()[j] = weights[j + num_full_ctxt * num_coeff];
            }
        }
        else
        {
            if (weights[j + num_full_ctxt * num_coeff] > 0)
            {
                w[num_full_ctxt].data()[poly_modulus_degree - j] = plain_modulus - weights[j + num_full_ctxt * num_coeff];
            }
            else
            {
                w[num_full_ctxt].data()[poly_modulus_degree - j] = -weights[j + num_full_ctxt * num_coeff];
            }
        }
    }

    vector<Ciphertext> cipherweight(num_full_ctxt + 1);
    cout << "\nEncrypting model..." << endl;
    for (int j = 0; j < num_full_ctxt+1; j++) {
        encryptor_pub.encrypt(w[j], cipherweight[j]);
    }
    
    cout << "Encryption done" << endl;
    /// SAVING ENCRYPTED MODEL
    cout << "save ctxt into " << modelname << endl;
    SaveVec(cipherweight, modelname);
    auto end_enc_model = std::chrono::high_resolution_clock::now();


    auto start_eval = std::chrono::high_resolution_clock::now();
    cout << "\n-----------EVALUATION (Evaluator) -----------\n"
         << endl;
    //// LOADING ENCRYPTED GENOTYPE AND MODEL
    cout << "load ctxts from " << filename << endl;
    vector<vector<Ciphertext>> genocipher = LoadGenotype(filename, context, num_samples, num_full_ctxt + 1);
    cout << "load ctxts from " << modelname << endl;
    vector<Ciphertext> weightcipher = LoadVec(modelname, context, num_full_ctxt + 1);
    vector<Ciphertext> predictions;
    for (int sample = 0; sample < num_samples; sample++)
    {
        vector<Ciphertext> wg(num_full_ctxt + 1);
        for (int j = 0; j < num_full_ctxt + 1; j++) {
            evaluator.multiply(weightcipher[j], genocipher[sample][j], wg[j]);
        }
        for (int j = 1; j < num_full_ctxt + 1; j++) {
            evaluator.add_inplace(wg[0], wg[j]);
        }
        predictions.push_back(wg[0]);
    }
    //// SAVING RESULTS
    string resultsname = "../savefiles/cipherresults" + phenotype + ".dat";
    cout << "save results into " << resultsname << endl;
    SaveVec(predictions, resultsname);
    auto end_eval = std::chrono::high_resolution_clock::now();

    auto start_dec = std::chrono::high_resolution_clock::now();
    cout << "\n-----------DECRYPTION (Client) -----------\n"
         << endl;
    
    cout << "load ctxts from " << resultsname << endl;
    vector<Ciphertext> resultciphers = LoadVec(resultsname, context, num_samples);
    vector<Plaintext> pt_result;
    for (int sample = 0; sample < num_samples; sample++)
    {
        Plaintext result;

        decryptor.decrypt(resultciphers[sample], result);

        pt_result.push_back(result);
    }
    auto end_dec = std::chrono::high_resolution_clock::now();


    cout << "\n-----------Saving RESULTS-----------\n"
         << endl;
    vector<double> final_result;
    for (int sample = 0; sample < 198; sample++)
    {
        double final;
        if (pt_result[sample].data()[0] > plain_modulus / 2)
        {
            final = -(double)(plain_modulus - pt_result[sample].data()[0]) / (double)scaling;
        }
        else
        {
            final = (double)pt_result[sample].data()[0] / (double)scaling;
        }
        final_result.push_back(final);
    }

    ofstream ofs;

    SaveResult(final_result, "../output/out_model" + phenotype + ".csv");

    auto duration_enc_geno = std::chrono::duration_cast<std::chrono::milliseconds>(end_enc_geno - start_enc_geno);
    auto duration_enc_model = std::chrono::duration_cast<std::chrono::milliseconds>(end_enc_model - start_enc_model);
    auto duration_eval = std::chrono::duration_cast<std::chrono::milliseconds>(end_eval - start_eval);
    auto duration_dec = std::chrono::duration_cast<std::chrono::milliseconds>(end_dec - start_dec);
    auto total_time = duration_enc_geno.count() + duration_enc_model.count() + duration_eval.count() + duration_dec.count();
    cout << "enc_geno time (ms) : " << duration_enc_geno.count() << endl;
    cout << "enc_model time (ms) : " << duration_enc_model.count() << endl;
    cout << "eval time (ms) : " << duration_eval.count() << endl;
    cout << "dec time (ms) : " << duration_dec.count() << endl;
    cout << endl;
    cout << "TOTAL time (ms) : " << total_time << endl;
}  