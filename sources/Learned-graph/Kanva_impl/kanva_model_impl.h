#ifndef KANVA_KANVA_MODEL_IMPL_H
#define KANVA_KANVA_MODEL_IMPL_H
#include "kanva_model.h"
#include "../common/util.h"
#include "../snapcollector.h"


void reportEdge(Enode<val_type> *victim, Vnode <val_type> * source_enode, int tid, int action, fstream *logfile, bool debug);
void reportVertex(Vnode<val_type> *victim,int tid, int action, fstream * logfile, bool debug);

template<class key_t, class val_t>
KanvaModel<key_t, val_t>::KanvaModel(){
    model = nullptr;
    maxErr = 64;
    err = 0;
    keys = nullptr;
    capacity = 0;
}

template<class key_t, class val_t>
KanvaModel<key_t, val_t>::~KanvaModel()
{
    if(model) model=nullptr;
}

template<class key_t, class val_t>
KanvaModel<key_t, val_t>::KanvaModel(lrmodel_type &lrmodel,
                                     const typename std::vector<key_t>::const_iterator &keys_begin,
                                     const typename std::vector<val_t>::const_iterator &vals_begin,
                                     size_t size, size_t _maxErr) : maxErr(_maxErr), capacity(size)
{
    model=new lrmodel_type(lrmodel.get_weight0(), lrmodel.get_weight1());
    keys = (key_t *)malloc(sizeof(key_t)*size);
//    vals = (val_t *)malloc(sizeof(val_t)*size);
    vals = new std::atomic<val_t>[size];
    valid_flag = (bool*)malloc(sizeof(bool)*size)
            ;
    for(int i=0; i<size; i++){
        keys[i] = *(keys_begin+i);
        vals[i] = {*(vals_begin+i)};
        valid_flag[i] = true;
    }
    mobs_lf = (std::atomic<struct model_or_bin<key_t, val_t>*>*)malloc(sizeof(std::atomic<struct model_or_bin<key_t, val_t>*>)*(size+1));
    for(int i=0; i<size+1; i++){
        mobs_lf[i]=nullptr;
    }
}


template<class key_t, class val_t>
key_t  KanvaModel<key_t, val_t>::get_keys(key_type i)
{
    return keys[i];
}

template<class key_t, class val_t>
val_t  KanvaModel<key_t, val_t>::get_vals(key_type i)
{
    return vals[i].load();
}

template<class key_t, class val_t>
struct model_or_bin<key_t, val_t> * KanvaModel<key_t, val_t>::get_mobs(int i)
{
    return mobs_lf[i].load();
}

template<class key_t, class val_t>
int KanvaModel<key_t, val_t>::get_capacity()
{
    return capacity;
}
// =====================  print =====================
template<class key_t, class val_t>
inline void KanvaModel<key_t, val_t>::print_model()
{
    model->print_weights();
    print_keys();
}
//
//template<class key_t, class val_t>
//void KanvaModel<key_t, val_t>::print_keys()
//{
//    if(mobs_lf[0]){
//        if(mobs_lf[0].load()->isbin)
//            mobs_lf[0].load()->mob.lflb->print(std::cout);
//        else
//            mobs_lf[0].load()->mob.ai->print_keys();
//    }
//
//    for(size_t i=0; i<capacity; i++){
//        std::cout<<"keys["<<i<<"]: " <<keys[i] << std::endl;
//        if(mobs_lf[i+1]){
//            if(mobs_lf[i+1].load()->isbin)
//                mobs_lf[i+1].load()->mob.lflb->print(std::cout);
//            else
//                mobs_lf[i+1].load()->mob.ai->print_keys();
//        }
//    }
//}

//template<class key_t, class val_t>
//void KanvaModel<key_t, val_t>::print_model_retrain()
//{
//    std::cout<<"[print aimodel] capacity:"<<capacity<<" -->";
//    model->print_weights();
//    if(mobs_lf[0]) {
//        if(mobs_lf[0].load()->isbin){
//            mobs_lf[0].load()->mob.lb->print(std::cout);
//        }else {
//            mobs_lf[0].load()->mob.ai->print_model_retrain();
//        }
//    }
//    for(size_t i=0; i<capacity; i++){
//        std::cout<<"keys["<<i<<"]: " <<keys[i] << std::endl;
//        if(mobs_lf[i+1]) {
//            if(mobs_lf[i+1].load()->isbin){
//                mobs_lf[i+1].load()->mob.lb->print(std::cout);
//            }else {
//                mobs_lf[i+1].load()->mob.ai->print_model_retrain();
//            }
//        }
//    }
//}

//template<class key_t, class val_t>
//void KanvaModel<key_t, val_t>::self_check_retrain()
//{
//    for(size_t i=1; i<capacity; i++){
//        assert(keys[i]>keys[i-1]);
//    }
//    for(size_t i=0; i<=capacity; i++){
//        model_or_bin_t *mob = mobs_lf[i];
//        if(mob){
//            if(mob->isbin){
//                mob->mob.lb->self_check();
//            } else {
//                mob->mob.ai->self_check_retrain();
//            }
//        }
//    }
//}

// ============================ rangequery =====================
template<class key_t, class val_t>
int KanvaModel<key_t, val_t>::rangequery(const key_t &key, const int n,
                                         std::vector<std::pair<key_t, val_t>> &result) {
    size_t remaining = n;
    size_t pos = predict(key);
    pos = locate_in_levelbin(key, pos);
    while (remaining > 0 && pos <= capacity) {
        if (pos < capacity && keys[pos] >= key) {
            result.push_back(std::pair<key_t, val_t>(keys[pos], vals[pos]));
            remaining--;
            if (remaining <= 0) break;
        }
        if (mobs_lf[pos] != nullptr) {
            struct model_or_bin<key_t, val_t> *mob = mobs_lf[pos];
            if (mob->isbin) {
                remaining = mob->mob.lflb->range_query(key, remaining, result);
            } else {
                remaining = mob->mob.ai->rangequery(key, remaining, result);
            }
        }
        pos++;
    }
    return remaining;
}




// ============================ search =====================

template<class key_t, class val_t>
val_t KanvaModel<key_t, val_t>::find_retrain(const key_t &key, val_t &val,int thread_num)
{
    size_t pos = predict(key);
    pos = locate_in_levelbin(key, pos);
    if(key == keys[pos]){

        return vals[pos];
    }
    int bin_pos = key<keys[pos]?pos:(pos+1);
    struct model_or_bin<key_t, val_t>* mob;
    mob = mobs_lf[bin_pos].load(std::memory_order_seq_cst);

    if(mob==nullptr) return -1;
    if(mob-> isbin)
        return mob->mob.lflb->search(key,thread_num);
    else
        return mob->mob.ai->find_retrain(key, val,thread_num);
}

/**
 * @brief find the value of the key
 *
 * @param key
 * @return val_t
 * @note The function return Vnode pointer if the key is present else nullptr
 * the caller should check if the node is marked or not
 */
template<>
Vnode<val_type>* KanvaModel<key_type, Vnode<val_type>*>::find_retrain(const key_type &key, Vnode<val_type>* &val,int thread_num)
{
    size_t pos = predict(key);
    pos = locate_in_levelbin(key, pos);
    if(key == keys[pos]){
        Vnode<val_type>* temp = vals[pos];
        if(is_marked_ref((long)temp->vnext.load())) {
            reportVertex(temp , thread_num , 1, nullptr, false);
            return nullptr;
        }
        else
        {
            return temp;
        }
    }
    int bin_pos = key<keys[pos]?pos:(pos+1);
    struct model_or_bin<key_type , Vnode<val_type>*>* mob;
    mob = mobs_lf[bin_pos].load(std::memory_order_seq_cst);

    if(mob==nullptr) return nullptr;
    if(mob-> isbin)
        return mob->mob.lflb->search(key,thread_num);
    else
        return mob->mob.ai->find_retrain(key, val,thread_num);
}






template<class key_t, class val_t>
inline size_t KanvaModel<key_t, val_t>::predict(const key_t &key) {
    size_t index_pos = model->predict(key);
    return index_pos < capacity? index_pos:capacity-1;
}

// =============================== insert =======================
template<class key_t, class val_t>
bool KanvaModel<key_t, val_t>::insert_retrain(const key_t &key, val_t val, int thread_num)
{
    size_t pos = predict(key);
    pos = locate_in_levelbin(key, pos);

    if(key == keys[pos]){
        if(vals[pos] != -1)
            return false;
        else
        {
            vals[pos] = val;
            return true;
        }
    }

    int bin_pos = pos;
    bin_pos = key<keys[bin_pos]?bin_pos:(bin_pos+1);
    return insert_model_or_bin(key, val, bin_pos);
}
template<>
bool KanvaModel<key_type, Vnode<val_type>*>::insert_retrain(const key_type &key, Vnode<val_type>* val, int thread_num)
{
    size_t pos = predict(key);
    pos = locate_in_levelbin(key, pos);

    if(key == keys[pos]){
        Vnode<val_type>* temp = vals[pos];
        if(is_marked_ref((long)temp->vnext.load())) {
            reportVertex(temp , thread_num , 1, nullptr, false);
            return false;
        }
        else
        {
            //update the value
            Vnode<val_type>* temp = vals[pos];
            vals[pos].compare_exchange_strong(temp, val);
            return true;
        }
    }

    int bin_pos = pos;
    bin_pos = key<keys[bin_pos]?bin_pos:(bin_pos+1);
    return insert_model_or_bin(key, val, bin_pos,thread_num);
}


template<class key_t, class val_t>
bool KanvaModel<key_t, val_t>::insert_model_or_bin(const key_t &key, const val_t &val, size_t bin_pos,int thread_num )
{
    struct model_or_bin<key_t, val_t> *mob = mobs_lf[bin_pos];
    retry:
    if(mob==nullptr){
        struct model_or_bin<key_t, val_t> *new_mob = new struct model_or_bin<key_t, val_t>();
        new_mob->mob.lflb = new Bin<key_t, val_t>();
        new_mob->isbin = true;
        if(!mobs_lf[bin_pos].compare_exchange_strong(mob, new_mob, std::memory_order_seq_cst, std::memory_order_seq_cst)){
            delete new_mob;
            goto retry;
        }
        mob = new_mob;
    }
    assert(mob!=nullptr);
    if(mob->isbin) {           // insert into bin
        bool res = mob->mob.lflb->insert(key , val , thread_num);
        if(!res){
            std::vector<key_t> retrain_keys;
            std::vector<val_t> retrain_vals;
            mob->mob.lflb->collect(retrain_keys, retrain_vals,thread_num);
            lrmodel_type model;
            model.train(retrain_keys.begin(), retrain_keys.size());
            size_t err = model.get_maxErr();
            kanva_model_type *ai = new kanva_model_type(model, retrain_keys.begin(), retrain_vals.begin(), retrain_keys.size(), err);
            struct model_or_bin<key_t, val_t>* new_mob = new struct model_or_bin<key_t, val_t>();
            new_mob -> mob.ai = ai;
            new_mob->isbin = false;
            if(!mobs_lf[bin_pos].compare_exchange_strong(mob, new_mob)) {
                delete new_mob;
                goto retry;
            }
            res = ai->insert_retrain(key, val,thread_num);
            return res;
        }
        else
            return res;
    } else{                   // insert into model
        return mob->mob.ai->insert_retrain(key, val, thread_num);
    }
}

// ========================== remove =====================
template<class key_t, class val_t>
bool KanvaModel<key_t, val_t>::remove(const key_t &key,int thread_num)
{
    size_t pos = predict(key);
    pos = locate_in_levelbin(key, pos,thread_num);
    if (key == keys[pos]) {
        vals[pos] = -1;
        return true;
    }
    int bin_pos = key<keys[pos]?pos:(pos+1);
    return remove_model_or_bin(key, bin_pos,thread_num);
}

template<>
bool KanvaModel<key_type , Vnode<val_type>*>::remove(const key_type &key,int thread_num)
{
    size_t pos = predict(key);
    pos = locate_in_levelbin(key, pos);
    if (key == keys[pos]) {
        //mark the vnect of node
        Vnode<val_type>* vnode = vals[pos].load();
        Vnode<val_type > *tmp = end_Vnode_T;
        int ret = vnode->vnext.compare_exchange_strong(tmp, (Vnode<val_type>*)set_mark((long) end_Vnode_T));
        reportVertex(tmp , thread_num , 1, nullptr, false);
        return ret;

    }
    int bin_pos = key<keys[pos]?pos:(pos+1);
    return remove_model_or_bin(key, bin_pos,thread_num);
}

template<class key_t, class val_t>
bool KanvaModel<key_t, val_t>::remove_model_or_bin(const key_t &key, const int bin_pos,int thread_num)
{
    retry:
    struct model_or_bin<key_t, val_t>* mob = mobs_lf[bin_pos];
    if(mob==nullptr) return false;
    if (mob->isbin) {
        int res;
        res = mob->mob.lflb->remove(key,thread_num);
        if(res == -2)
            return false;
        else if(res == -1)
        {
            std::vector<key_t> retrain_keys;
            std::vector<val_t> retrain_vals;
            mob->mob.lflb->collect(retrain_keys, retrain_vals,thread_num);
            lrmodel_type model;
            model.train(retrain_keys.begin(), retrain_keys.size());
            size_t err = model.get_maxErr();
            kanva_model_type *ai = new kanva_model_type (model, retrain_keys.begin(), retrain_vals.begin(), retrain_keys.size(), err);
            struct model_or_bin<key_t, val_t>* new_mob = new struct model_or_bin<key_t, val_t>();
            new_mob -> mob.ai = ai;
            new_mob->isbin = false;
            if(!mobs_lf[bin_pos].compare_exchange_strong(mob, new_mob)) {
                delete new_mob;
                goto retry;
            }
            return ai->remove(key,thread_num);
        }
        else
            return true;
    } else {
        return mob->mob.ai->remove(key,thread_num);
    }
}

template<class key_t, class val_t>
inline size_t KanvaModel<key_t, val_t>::locate_in_levelbin(const key_t &key, const size_t pos)
{
    // predict
    //size_t index_pos = model->predict(key);
    size_t index_pos = pos;
    size_t upbound = capacity-1;
    //index_pos = index_pos <= upbound? index_pos:upbound;

    // search
    size_t begin, end, mid;
    if(key > keys[index_pos]){
        begin = index_pos+1 < upbound? (index_pos+1):upbound;
        end = begin+maxErr < upbound? (begin+maxErr):upbound;
    } else {
        end = index_pos;
        begin = end>maxErr? (end-maxErr):0;
    }

    assert(begin<=end);
    while(begin != end){
        mid = (end + begin+2) / 2;
        if(keys[mid]<=key) {
            begin = mid;
        } else
            end = mid-1;
    }
    return begin;
}
#endif //KANVA_KANVA_MODEL_IMPL_H
