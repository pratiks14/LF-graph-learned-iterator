#ifndef KANVA_KANVA_H
#define KANVA_KANVA_H
#include "../common/util.h"
#include "../common/lr_model.h"
#include "../common/lr_model_impl.h"
#include "kanva_model.h"
#include "kanva_model_impl.h"


template<class key_t, class val_t>
class Kanva{
public:
    typedef KanvaModel<key_t, val_t> kanvamodel_type;
    typedef LinearRegressionModel<key_t> lrmodel_type;

public:
    inline Kanva();
    inline Kanva(int _maxErr, int _learning_step, float _learning_rate);
    ~Kanva();
    void train(const std::vector<key_t> &keys, const std::vector<val_t> &vals, size_t _maxErr);
    void train_opt(const std::vector<key_t> &keys, const std::vector<val_t> &vals, size_t _maxErr);
    //void retrain(typename root_type::iterator it);
    void print_models();
    void self_check();
    int rangequery(const key_t &key, const int n, std::vector<std::pair<key_t, val_t>> &result);


    inline val_t find(const key_t &key, val_t &val);
    inline bool insert(const key_t &key, const val_t &val);
    inline bool remove(const key_t &key);
    bool fetch_vertices(val_t  *n1, val_t *n2, key_t key1, key_t key2);
    bool fetch_vertices_help(val_t *n1, val_t *n2, key_t key1, key_t key2);
    int ContainsE(key_t key1, key_t key2, int tid, fstream *logfile, bool debug);
    int AddEdge(key_t key1, key_t key2, int tid, fstream *logfile, bool debug);
    void locateE(Vnode<val_type> **source_of_edge, Enode<val_type> **n1, Enode<val_type> **n2, int key, int tid, fstream *logfile, bool debug);
    int RemoveE(key_t key1, key_t key2, int tid, fstream *logfile, bool debug);
    size_t model_size();
    vector<key_type> get_keys();
    vector<kanvamodel_type> get_aimodels();

        private : size_t
                  backward_train(const typename std::vector<key_t>::const_iterator &keys_begin,
                                 const typename std::vector<val_t>::const_iterator &vals_begin,
                                 uint32_t size, int step);
    void append_model(lrmodel_type &model, const typename std::vector<key_t>::const_iterator &keys_begin,
                      const typename std::vector<val_t>::const_iterator &vals_begin,
                      size_t size, int err);
    kanvamodel_type * find_model(const key_t &key);
    int locate_in_levelbin(key_t key, int model_pos);


private:
    std::vector<key_t> model_keys;
    std::vector<kanvamodel_type> aimodels;
    int maxErr = 64;
    int learning_step = 1000;
    float learning_rate = 0.1;
};


#endif //KANVA_KANVA_H
