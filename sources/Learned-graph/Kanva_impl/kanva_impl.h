#ifndef KANVA_KANVA_IMPL_H
#define KANVA_KANVA_IMPL_H
#include "kanva.h"
#include "kanva_model.h"
#include "kanva_model_impl.h"

template<class key_t, class val_t>
inline Kanva<key_t, val_t>::Kanva()
{
    maxErr = 64;
    learning_rate = 0.1;
    learning_step = 1000;
}

template<class key_t, class val_t>
inline Kanva<key_t, val_t>::Kanva(int _maxErr, int _learning_step, float _learning_rate)
        : maxErr(_maxErr), learning_step(_learning_step), learning_rate(_learning_rate)
{}

template<class key_t, class val_t>
Kanva<key_t, val_t>::~Kanva(){}

// ====================== train models ========================
template<class key_t, class val_t>
void Kanva<key_t, val_t>::train(const std::vector<key_t> &keys,
                                const std::vector<val_t> &vals, size_t _maxErr)
{
    assert(keys.size() == vals.size());
    maxErr = _maxErr;
//    std::cout<<"training begin, length of training_data is:" << keys.size() <<" ,maxErr: "<< maxErr << std::endl;

    size_t start = 0;
    size_t end = learning_step<keys.size()?learning_step:keys.size();
    while(start<end){
        //COUT_THIS("start:" << start<<" ,end: "<<end);
        lrmodel_type model;
        model.train(keys.begin()+start, end-start);
        size_t err = model.get_maxErr();
        // equal
        if(err == maxErr) {
            append_model(model, keys.begin()+start, vals.begin()+start, end-start, err);
        } else if(err < maxErr) {
            if(end>=keys.size()){
                append_model(model, keys.begin()+start, vals.begin()+start, end-start, err);
                break;
            }
            end += learning_step;
            if(end>keys.size()){
                end = keys.size();
            }
            continue;
        } else {
            size_t offset = backward_train(keys.begin()+start, vals.begin()+start, end-start, int(learning_step*learning_rate));
            end = start + offset;
        }
        start = end;
        end += learning_step;
        if(end>=keys.size()){
            end = keys.size();
        }
    }

    //root = new root_type(model_keys);
//    COUT_THIS("[aidle] get models -> "<< model_keys.size());
    assert(model_keys.size()==aimodels.size());
}
template<class key_t, class val_t>
std::vector<key_type> Kanva<key_t, val_t>::get_keys()
{
    return model_keys;
}

template <class key_t, class val_t>
std::vector<KanvaModel<key_t,val_t>> Kanva<key_t, val_t>::get_aimodels(){
    return aimodels;
}

    template <class key_t, class val_t>
    size_t Kanva<key_t, val_t>::backward_train(const typename std::vector<key_t>::const_iterator &keys_begin,
                                               const typename std::vector<val_t>::const_iterator &vals_begin,
                                               uint32_t size, int step)
{
    if(size<=10){
        size = 10;
        step = 1;
    } else {
        while(size<=step){
            step = int(step*learning_rate);
        }
    }
    assert(step>0);
    int start = 0;
    int end = size-step;
    while(end>0){
        lrmodel_type model;
        model.train(keys_begin, end);
        size_t err = model.get_maxErr();
        if(err<=maxErr){
            append_model(model, keys_begin, vals_begin, end, err);
            return end;
        }
        end -= step;
    }
    end = backward_train(keys_begin, vals_begin, end, int(step*learning_rate));
    return end;
}

template<class key_t, class val_t>
void Kanva<key_t, val_t>::append_model(lrmodel_type &model,
                                       const typename std::vector<key_t>::const_iterator &keys_begin,
                                       const typename std::vector<val_t>::const_iterator &vals_begin,
                                       size_t size, int err)
{
    key_t key = *(keys_begin+size-1);

    // set learning_step
    int n = size/10;
    learning_step = 1;
    while(n!=0){
        n/=10;
        learning_step*=10;
    }

    assert(err<=maxErr);
    kanvamodel_type aimodel(model, keys_begin, vals_begin, size, maxErr);

    model_keys.push_back(key);
    aimodels.push_back(aimodel);
    if(learning_step == 1)
        learning_step = 10;
}

template<class key_t, class val_t>
typename Kanva<key_t, val_t>::kanvamodel_type * Kanva<key_t, val_t>::find_model(const key_t &key)
{
    // root
    size_t model_pos = binary_search_branchless(&model_keys[0], model_keys.size(), key);
    if(model_pos >= aimodels.size())
        model_pos = aimodels.size()-1;
    return &aimodels[model_pos];
}


// ===================== print data =====================
template<class key_t, class val_t>
void Kanva<key_t, val_t>::print_models()
{
    for(int i=0; i<model_keys.size(); i++){
        std::cout<<"model "<<i<<" ,key:"<<model_keys[i]<<" ->";
        aimodels[i].print_model();
    }
}

template<class key_t, class val_t>
void Kanva<key_t, val_t>::self_check()
{
    for(int i=0; i<model_keys.size(); i++){
        aimodels[i].self_check();
    }

}


// =================== search the data =======================
template<class key_t, class val_t>
inline val_t Kanva<key_t, val_t>::find(const key_t &key, val_t &val)
{
      return find_model(key)[0].find_retrain(key, val);
}

template<class key_t, class val_t>
inline val_t Kanva<key_t, val_t>::find_help(const key_t &key, val_t &val)
{
    return find_model(key)[0].find_retrain_help(key, val);
}




// =================  scan ====================
template<class key_t, class val_t>
int Kanva<key_t, val_t>::rangequery(const key_t &key, const int n,
                                    std::vector<std::pair<key_t, val_t>> &result) {
    size_t remaining = n;
    size_t model_pos = binary_search_branchless(&model_keys[0], model_keys.size(), key);
    if(model_pos >= aimodels.size())
        model_pos = aimodels.size()-1;
    while(remaining>0 && model_pos < aimodels.size()){
        remaining = aimodels[model_pos].rangequery(key, remaining, result);
//            std::cout<<"Remaining Value returned in RQ1:  "<<remaining<<"\n";
    }
    return remaining;
}

// =================== insert the data =======================
template<class key_t, class val_t>
inline bool Kanva<key_t, val_t>::insert(
        const key_t& key, const val_t& val)
{
    return find_model(key)[0].insert_retrain(key, val);
}



// ==================== remove =====================
template<class key_t, class val_t>
inline bool Kanva<key_t, val_t>::remove(const key_t& key)
{
    return find_model(key)[0].remove(key);

    //return find_model(key)[0].con_insert(key, val);
}


//New Methods for Kanva for val type Vnodes with edges

//======================= contains Edge ========================

template<class key_t, class val_t>
bool Kanva<key_t, val_t>::fetch_vertices(val_t  *n1, val_t *n2, key_t key1, key_t key2) {

    val_t tmp;//will not be uused should be removed later
    val_t node = find(key1, tmp);

    if constexpr(std::is_pointer_v<val_t>)
    {
        if(node == nullptr) {
            return false;
        }
    }
    else if(node == -1)
        return false;

    *n1 = node;

    node = find(key2, tmp);
    if constexpr(std::is_pointer_v<val_t>) {
        if(node == nullptr) {
            return false;
        }
    }
    else if(node == -1) {
        return false;
    }
    *n2 = node;

    return true;
}

template <class key_t, class val_t>
bool Kanva<key_t, val_t>::fetch_vertices_help(val_t *n1, val_t *n2, key_t key1, key_t key2)
{

    val_t tmp; // will not be uused should be removed later
    val_t node = find_help(key1, tmp);

    if constexpr (std::is_pointer_v<val_t>)
    {
        if (node == nullptr)
        {
            return false;
        }
    }
    else if (node == -1)
        return false;

    *n1 = node;

    node = find_help(key2, tmp);
    if constexpr (std::is_pointer_v<val_t>)
    {
        if (node == nullptr)
        {
            return false;
        }
    }
    else if (node == -1)
    {
        return false;
    }
    *n2 = node;

    return true;
}

template<>
int Kanva<key_type, Vnode<val_type>*>::ContainsE(key_type key1, key_type key2, int tid, fstream *logfile, bool debug) {
        Enode<val_type> * curre, * prede;
        Vnode<val_type> *u,  *v;
        bool flag = fetch_vertices( & u, & v, key1, key2);

        if (!flag) {
            return 1; // either of the vertex is not present
        }

        curre = u -> ehead.load();

        while (curre != end_Enode_T && curre -> val < key2) {
            curre = (Enode<val_type> * ) get_unmarked_ref((long) curre -> enext.load());
        }
        if ((curre) && curre -> val == key2
            && !is_marked_ref((long) curre -> enext.load())
            && !is_marked_ref((long) u -> vnext.load())
            && !is_marked_ref((long) curre->v_dest-> vnext.load())) {
            //reportEdge(curre ,u , tid, 2 ,logfile,debug);//
            return 2;
        } else {
            if (is_marked_ref((long) u)) {
                if(debug)
                    (*logfile) << "Source vertex : " << u->val << "(" << u << ")" <<" marked" << endl;
                //reportVertex(u , tid , 1,logfile, debug);//
            } else if (is_marked_ref((long) v)) {
                if(debug)
                    (*logfile) << "Destination vertex : " << v->val << "(" << v << ")" <<" marked" << endl;
                //reportVertex(v, tid , 1, logfile, debug);//
            } else if (is_marked_ref((long) curre -> enext.load())) {
                if(debug)
                    (*logfile) << "Edge marked : " << u->val << " " << curre->val << "(" << curre << ")" <<" marked" << endl;
                //reportEdge(curre , u , tid , 1,logfile,debug);//
            }
            return 3;
        }
    }


//======================= Insert Edge ========================
    template<>
    void Kanva<key_type, Vnode<val_type>*>::locateE(Vnode<val_type> **source_of_edge, Enode<val_type> **n1, Enode<val_type> **n2, int key, int tid, fstream *logfile, bool debug)
    {
        Enode<val_type> *succe, *curre, *prede;
        Vnode<val_type> *tv;
    retry:
        while (true)
        {

            prede = (*source_of_edge)->ehead.load();
            curre = prede->enext.load();

            while (true)
            {
                succe = curre->enext.load();
                tv = curre->v_dest;
                /*helping: delete one or more enodes whose vertex was marked*/
                // checking whether the destination vertex is marked (the next edge shouldn't be marked)
                // Note : Removed "tv" conditions
                /*helping: delete one or more enodes which are marked*/

                while (curre != end_Enode_T &&
                       (is_marked_ref((long)tv->vnext.load()) || is_marked_ref((long)succe)) && curre->val <= key)
                {
                    //reportEdge(curre, *source_of_edge, tid, 1, logfile, debug);
                    //report current node as marked if not already marked or if vdest is marked

                    if (!is_marked_ref((long)succe) and !atomic_compare_exchange_strong(&curre->enext, &succe, (Enode<val_type> *)get_marked_ref((long)succe)))
                        goto retry;

                    // physical deletion of enode if already marked
                    // Note : remove goto retry if physical deletion fails
                    if (!atomic_compare_exchange_strong(&prede->enext, &curre, (Enode<val_type> *)get_unmarked_ref((long)succe)))
                        goto retry;

                    curre = (Enode<val_type> *)get_unmarked_ref((long)succe);
                    succe = curre->enext.load();
                    tv = curre->v_dest;
                }

                // Note : Commented below 3 lines : not sure of the use of these
                // if (is_marked_ref((long) tv -> vnext.load()) &&
                //     curre != end_Enode && curre -> val < key)
                //     goto retry2;
                if (curre == end_Enode_T || curre->val >= key)
                {

                    (*n1) = prede;
                    (*n2) = curre;
                    return;
                }
                prede = curre;
                curre = (Enode<val_type> *)get_unmarked_ref((long)succe);
            }
        }
    }

    // add a new edge in the edge-list
    // returns 1 if vertex not present, 2 if edge already present and 3 if edge added
    template<>
    int Kanva<key_type, Vnode<val_type>*>::AddEdge(key_type key1, key_type key2, int tid, fstream *logfile, bool debug)
    {
        Enode<val_type> *prede, *curre;
        Vnode<val_type> *u, *v;
        bool flag = fetch_vertices_help(&u, &v, key1, key2);//within this if Vnode is found to be marked then its reported
        if (flag == false)
        {
            return 1; // either of the vertex is not present
        }
        // cout << key1 <<endl;
        // cout << key2 << endl;
        while (true)
        {
            if (is_marked_ref((long)u->vnext.load()))
            {
                //reportVertex(u, tid, 1, logfile, debug); //
                return 1;                                // either of the vertex is not present
            }
            else if (is_marked_ref((long)v->vnext.load()))
            {
                //reportVertex(v, tid, 1, logfile, debug); //
                return 1;                                // either of the vertex is not present
            }

            locateE(&u, &prede, &curre, key2, tid, logfile, debug);

            if (curre->val == key2)
            {
                if (debug)
                    (*logfile) << "Edge : " << key1 << " " << key2 << " already present" << endl;
                //reportEdge(curre, u, tid, 2, logfile, debug); //
                return 2;                                     // edge already present
            }
            Enode<val_type> *newe = new Enode<val_type>(key2, v, curre);//creating new edge

            if (atomic_compare_exchange_strong(&prede->enext, &curre, newe)) // insertion
            {
                if (debug)
                    (*logfile) << "New Edge added  : " << key1 << " " << key2 << "(" << newe << ")" << endl;
                //reportEdge(newe, u, tid, 2, logfile, debug); //
                return 3;
            }
            delete newe;
        } // End of while
    }


    //======================= Remove Edge ========================
    template<>
    int Kanva<key_type, Vnode<val_type> *>::RemoveE(key_type key1, key_type key2, int tid, fstream *logfile, bool debug)
    {
        Enode<val_type> *prede, *curre, *succe;
        Vnode<val_type> *u, *v;
        bool flag = fetch_vertices_help(&u, &v, key1, key2);
        if (flag == false)
        {
            return 1; // either of the vertex is not present
        }

        while (true)
        {
            if (is_marked_ref((long)u->vnext.load()))
            {
                //reportVertex(u, tid, 1, logfile, debug); //
                return 1;
            }
            else if (is_marked_ref((long)v->vnext.load()))
            {
                //reportVertex(v, tid, 1, logfile, debug); //
                return 1;
            }
            locateE(&u, &prede, &curre, key2, tid, logfile, debug);

            if (curre->val != key2)
            {
                if (debug)
                    (*logfile) << "Edge not found  : " << key1 << " " << key2 << endl;
                return 2; // edge not present
            }
            succe = curre->enext.load();
            if (!is_marked_ref((long)succe))
            {
                if (!atomic_compare_exchange_strong(&curre->enext, &succe, (Enode<val_type> *)get_marked_ref((long)succe))) //  logical deletion
                {
                    continue;// if the comaapre and exchange fails then retry
                }
            }
            succe = (Enode<val_type> *)get_unmarked_ref((long)succe);
            atomic_compare_exchange_strong(&prede->enext, &curre, succe);// marked edge is physically deleted
            break;

        }
        return 3;
    }

#endif //KANVA_KANVA_IMPL_H
