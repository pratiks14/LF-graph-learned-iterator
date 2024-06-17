#include "Kanva_impl/kanva.h"
#include "Kanva_impl/kanva_impl.h"


int EXISTS_NUM = 1000000; // max number of keys to be sampled if the data contains less keys then all keys are sampled



void uniform_dense_1K_uint64(vector<key_type > &data1, vector<key_type > &exist_keys) {
    std::cout<<"data/uniform_dense_10K_uint64\n";
    std::string filename = "data/uniform_dense_10K_uint64";
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cout << "unable to open " << filename << std::endl;
        exit(EXIT_FAILURE);
    }
    key_type size;
    in.read(reinterpret_cast<char*>(&size), sizeof(key_type));
    data1.resize(size);
    // Read values.
    in.read(reinterpret_cast<char*>(data1.data()), size * sizeof(key_type));
    in.close();
    //std::cout << "Data Size  " << data1->size() << "\n";

    // Sample keys to be used for training the keys are ordered as in the data file
    std::sample(
            data1.begin(),
            data1.end(),
            std::back_inserter(exist_keys),
            EXISTS_NUM,
            std::mt19937{std::random_device{}()}
    );
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(data1.begin(), data1.end(), g);
}
void norm_dense_1K_uint64(vector<key_type > &data1, vector<key_type > &exist_keys) {
    std::cout<<"data/norm_dense_10K_uint64\n";
    std::string filename = "data/norm_dense_10K_uint64";
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cout << "unable to open " << filename << std::endl;
        exit(EXIT_FAILURE);
    }
    key_type size;
    in.read(reinterpret_cast<char*>(&size), sizeof(key_type));
    data1.resize(size);
    // Read values.
    in.read(reinterpret_cast<char*>(data1.data()), size * sizeof(key_type));
    in.close();
    //std::cout << "Data Size  " << data1->size() << "\n";

    // Sample keys to be used for training the keys are ordered as in the data file
    std::sample(
            data1.begin(),
            data1.end(),
            std::back_inserter(exist_keys),
            EXISTS_NUM,
            std::mt19937{std::random_device{}()}
    );
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(data1.begin(), data1.end(), g);
}
//create Vnode values curresponding to the keys
void create_values(vector<key_type > &exist_keys,vector<Vnode<val_type> *> &values) {
    //std::cout<<"create_values\n";
    
    for (int i = 0; i < exist_keys.size(); i++) {
        Enode<key_type> * EHead = new Enode<key_type>(0, NULL,end_Enode_T);
        Vnode<val_type> *v = new Vnode<val_type>(exist_keys[i],end_Vnode_T, EHead);
        values.push_back(v);
    }
}
Kanva<key_type ,Vnode<val_type>*>* prepare(Kanva<key_type ,Vnode<val_type>*> *km, const vector<key_type > &keys, const vector<Vnode<val_type> *> &values){
    //COUT_THIS("[Training Kanva]");
    double time_s = 0.0;
    size_t maxErr = 4;
//    TIMER_DECLARE(0);
//    TIMER_BEGIN(0);
    km->train(keys, values, 32);
//    TIMER_END_S(0,time_s);
//    printf("%8.1lf s : %.40s\n", time_s, "training");
    return km;
}
Kanva<key_type ,Vnode<val_type>*> * create_kanva_model(vector<key_type > &vertices  , vector<pair<key_type , key_type >> &edges){
//    vector<key_type>  exist_keys;
//    uniform_dense_1K_uint64(data1 , exist_keys); //read file and sample keys
//    norm_dense_1K_uint64(data1 , exist_keys); //read file and sample keys
    sort(vertices.begin(), vertices.end());
    vector<Vnode<val_type> *> values;
//
    create_values(vertices,values);
    Kanva<key_type , Vnode<val_type> *>*km = new Kanva<key_type , Vnode<val_type> *>();
//    cout << "Training started\n";
    prepare(km, vertices, values);
//    std::cout << "Training done\n";
    for(pair<key_type, key_type > edge : edges){
        km->AddEdge(edge.first, edge.second, 0, NULL, false);
    }


    return km;
}


