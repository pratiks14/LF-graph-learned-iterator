#ifndef SNAPCOLLECTOR_HEADER_H
#define SNAPCOLLECTOR_HEADER_H



#include "graphDS.h"
#include <vector>
#include "utility.h"
#include <atomic>
#include <algorithm>
#include <fstream>
#include<queue>
#include <chrono>
#include "Kanva_impl/kanva.h"
#include "Kanva_impl/kanva_impl.h"
#include "Kanva_impl/kanva_model_impl.h"
#include "Kanva_impl/kanva_model.h"
#include "Kanva_impl/Bin_LL/LF_LL.h"

using namespace std;

//template<class key_t, class val_t>
//class Kanva;

class SnapCollector;


atomic<SnapCollector *> PSC = {nullptr};
//int total_threads = 16;


bool flag = false;

class Snap_Vnode;

class Snap_Enode {
    public:
        atomic<Snap_Enode *> enext;
        int key;
        Enode<val_type> * enode;
        atomic<Snap_Vnode *> d_vnode; //dest_vnode
        int * visitedArray;// size same as threads // used to indicate whether the node has beeen visited by the given thread
        

        Snap_Enode(Enode<val_type> * enode, Snap_Enode * enext) {
            
            this -> enext = enext; 
            this -> enode = enode;
            this -> d_vnode = nullptr;
            d_vnode.store(nullptr);
            if(enode != nullptr)
                this->key = enode->val;
            
        }
        Snap_Enode(Enode<val_type> * enode, Snap_Enode * enext, Snap_Vnode * d_vnode) {
           
            this -> enext = enext; 
            this -> enode = enode;
            this -> d_vnode = d_vnode;
            if(enode != nullptr)
                this->key = enode->val;
            
        }

        ~Snap_Enode(){
            
        }
};
// the common sentinel snap Enode<val_type>
Snap_Enode * end_snap_Enode = new Snap_Enode(NULL, NULL);
typedef struct edge_report_info{//this will be created for each thread
    atomic<long> report_index = {-1};
}ereport_info;

// Structure for Collected graph's Vertex Node
class Snap_Vnode {
    public:
        atomic<Snap_Vnode *> vnext;
        Vnode<val_type> * vnode;
        Snap_Enode * ehead; // head of edge linked list
        atomic<int> edge_status ; //0->edges havent been processed by any thread 
                                    // 1-> being processed 2->completed processing
        atomic<int> iter_edge_status;
        atomic<long> report_index;

      
    //is_reconstruct is true then end enode is marked
    Snap_Vnode(Vnode<val_type> * vnode, Snap_Vnode * next_snap_vnode, bool is_reconstruct = false) {
        
        this -> vnode = vnode;
        this -> vnext = next_snap_vnode;
        Snap_Enode * start_snap_Enode;
        if (!is_reconstruct){
            start_snap_Enode = new Snap_Enode(vnode-> ehead, end_snap_Enode);
        }
        else
            start_snap_Enode = new Snap_Enode(vnode-> ehead, (Snap_Enode *)set_mark((long)end_snap_Enode));
        this -> ehead = start_snap_Enode;

        iter_edge_status = {0};
        edge_status = {0};
        report_index = {-1};
    }


    Snap_Vnode(Vnode<val_type> * vnode, Snap_Vnode * next_snap_vnode , Snap_Enode *end) {
        
        this -> vnode = vnode;
        this -> vnext = next_snap_vnode;
        Snap_Enode * start_snap_Enode = new Snap_Enode(this->vnode-> ehead, end_snap_Enode);
        this -> ehead = start_snap_Enode;

        edge_status = {0};
        report_index = {-1};
    }

    ~Snap_Vnode(){
        Snap_Enode * tmp = ehead;
        Snap_Enode * tmp_next = ehead->enext;
        delete tmp;
        while((Snap_Enode *)get_unmarked_ref((long)tmp_next) != end_snap_Enode){
            tmp = tmp_next;
            tmp_next = tmp_next->enext;
            delete tmp;
        }
    }
};

// the common sentinel snap Vnode<val_type>
Snap_Vnode *  end_snap_Vnode = new Snap_Vnode(end_Vnode_T, NULL);





// Vertex's Report Structure
class VertexReport {
    public:
        Vnode<val_type> * vnode; // here only value can be stored...used to sort the reports based on vertex or associated vertex in case of edge
        int action; // 1-insert 2-Delete 3-Block
        VertexReport * nextReport;

        VertexReport( Vnode<val_type> * vnode , int action, VertexReport *nextReport){
            this->vnode = vnode;
            this->action = action;
            this->nextReport = nextReport;
        };

        VertexReport(){
        }
};

// Edge's Report Structure
class EdgeReport {
    public:
        Enode<val_type> * enode;
        Vnode<val_type> * source; // here only value can be stored...used to sort the reports based on vertex or associated vertex in case of edge
        int action; // 1-insert 2-Delete 3-Block
        EdgeReport * nextReport;

        EdgeReport(Enode<val_type> * enode, Vnode<val_type>* source, int action, EdgeReport *nextReport){
            this->enode = enode;
            this->source = source;
            this->action = action;
            this->nextReport = nextReport;
        }
};

// BFSNode structure
typedef struct BFSNode{
	Snap_Vnode *n; // pointer to the VNode
	struct BFSNode *p; // pointer to the parent BFSNode
	struct BFSNode *next; // pointer to the next BFSNode
	struct BFSNode *back; // pointer to the Tail BFSNode
}bfslist_t;

// create BFSNode
//bfslist_t* createBFSNode(int ecount, Snap_Vnode *n, bfslist_t *p, bfslist_t *next){
//    bfslist_t * newbfs = (bfslist_t*) malloc(sizeof(bfslist_t));
//    newbfs->ecount = ecount;
//    newbfs ->n = n;
//    newbfs ->p = p;
//    newbfs ->next = next;
//    return newbfs;
//} 

bfslist_t* createBFSNode(Snap_Vnode *n, bfslist_t *p, bfslist_t *next){
    bfslist_t * newbfs = (bfslist_t*) malloc(sizeof(bfslist_t));
    newbfs ->n = n;
    newbfs ->p = p;
    newbfs ->next = next;
    newbfs ->back = NULL;
    return newbfs;
}


bool vertex_comparator(const VertexReport &lhs, const VertexReport &rhs)
{   

    if (lhs.vnode->val != rhs.vnode->val)
        return lhs.vnode->val < rhs.vnode->val;
    else if (lhs.vnode != rhs.vnode)
        return true;
    else
        return lhs.action < rhs.action; // delete report will have higher pref

}

bool edge_comparator(const EdgeReport &lhs, const EdgeReport &rhs)
{
    if (lhs.source->val != rhs.source->val)
        return lhs.source->val < rhs.source->val;
    if (lhs.enode->val != rhs.enode->val)
        return lhs.enode->val < rhs.enode->val;
    else if (lhs.enode != rhs.enode)
        return true;
    else
        return lhs.action < rhs.action; // delete report will have higher pref
}

/*for each thread a report will be maintained which cantains a linked list of edge report and vertex report*/
class Report{
    public :
        atomic<EdgeReport *>  head_edge_report;
        atomic<VertexReport *> head_vertex_report;
    
        Report(){
            head_edge_report = nullptr;
            head_vertex_report = nullptr;
        }
        ~Report(){
            EdgeReport * tmp_rep = head_edge_report.load();
            EdgeReport * next_tmp_rep;
            while(tmp_rep!= nullptr){
                next_tmp_rep = tmp_rep->nextReport;
                delete tmp_rep;
            }
            
            VertexReport * tmp_rep1 = head_vertex_report.load();
            VertexReport * next_tmp_rep1;
            while(tmp_rep1!= nullptr){
                next_tmp_rep1 = tmp_rep1->nextReport;
                delete tmp_rep1;
            }
            
        }
};



class SnapCollector{
    private:
        atomic<bool> active = {false};
    public :
        Kanva<key_type ,Vnode<val_type >*> * km;
        typedef struct aimodel_status
        {
            atomic <Snap_Vnode *> head ;
            atomic <Snap_Vnode *> tail ;
            atomic  <int> status;
            aimodel_status(){
                head = nullptr;
                tail = nullptr;
                status = 0;
            }
        } aimodel_status;
        //indicates if the snap collect field is currently active
        Snap_Vnode *head_snap_Vnode;//points to head of the collected vertex list
        atomic<Snap_Vnode *> tail_snap_E_V_ptr;//points to the vertex currently being iterated during edge iterations

        bool read_edge = false;// boolean value to indicate if that we are going through the edge
        Report **reports;//array of atomic report for each thread
        //vector <Report> delete_vertex_reports; //This will be used to check the while adding edges 
    
        int no_of_threads;
        atomic<long> vertex_iter_counter ;
        atomic<long> vertex_reconstruct_counter;
        atomic<aimodel_status *> aim_status_list = nullptr;
        //atomic<int> threads_accessing = {0} ; //no of threads accesssing the snapcollector

       

        //for reconstruction using report
    
        atomic<vector<VertexReport> *>sorted_vertex_reports_ptr = {nullptr};
        atomic<vector<EdgeReport> *>sorted_edge_reports_ptr = {nullptr};
        atomic<long> vertex_report_index = {0}; //used to store the highest index in sorted vertex reports currently being processed by any thread
        atomic<long> edge_report_index = {0} ;//at

        atomic<bool> reconstruction_completed;

        atomic<bool> iteration_completed;


        //Here head points to the "start_vnode" of the original graph
        SnapCollector(Vnode<val_type> *head, Kanva<key_type, Vnode<val_type> *> *km, int no_of_threads)
        {
            this->km = km;
            this->no_of_threads = no_of_threads;
            Snap_Vnode * start_snap_Vnode = new Snap_Vnode(head, (Snap_Vnode*)get_marked_ref((long)end_snap_Vnode) );
            head_snap_Vnode = start_snap_Vnode;
            tail_snap_E_V_ptr = nullptr;
            this->activate();

            reports = new Report*[no_of_threads];
            for( int i ; i < no_of_threads ;i++){
                reports[i] = new Report();
            }
            //++threads_accessing;
            
            reconstruction_completed = {false};
            iteration_completed = {false};
            vertex_iter_counter = {0};
            vertex_reconstruct_counter = {0};
        }

        ~SnapCollector(){
            Snap_Vnode * tmp = head_snap_Vnode;
            Snap_Vnode * tmp_next = head_snap_Vnode->vnext;

            delete tmp;
            while(get_unmarked_ref((long)tmp_next) != (long)end_snap_Vnode){
                tmp = tmp_next;
                tmp_next = tmp_next ->vnext;
                delete tmp;
            }
            
            vector<EdgeReport>  *a = sorted_edge_reports_ptr.load();
            if(a != nullptr)
                delete a;
            vector<VertexReport>  *b = sorted_vertex_reports_ptr.load();
            if(b != nullptr)
                delete b;
            
            for( int i ; i < no_of_threads ;i++){
                    delete reports[i];
            }
            delete[] aim_status_list;
        }

        //Note : 
        // start_snap_Vnode / end_snap_Vnode indicates the start and end of vertex list 
        // start_snap_Enode / end_snap_Enode indicates the start and end of edge list 
        // tail_snap_Vnode points to vertex which was last updated
        // tail_snap_Enode points to vertex which was last updated
        // snap_vertex_ptr is the vertex which we currently iterating while adding edges
    
        bool isActive(){
            return active.load();
        }

        void deactivate(){
            this->active = false;
        }
        void activate(){
            this->active = true;
        }

        int update_local_tail(aimodel_status *aim_stat, Snap_Vnode *&local_tail, val_type &find_key,fstream * logfile = nullptr, bool debug = false)
        {
           local_tail = aim_stat->tail.load();
//            if(local_tail == nullptr)
//                return -1;
            if(unset_mark((long)local_tail->vnext.load()) != (long)end_snap_Vnode)
            {
                Snap_Vnode * prev = local_tail;
                local_tail = prev->vnext.load();
                aim_stat->tail.compare_exchange_strong(prev, local_tail);

            }

            if((long)local_tail->vnext.load() == set_mark ((long)end_snap_Vnode))
            {
                int curr_status = 1;
               aim_stat->status.compare_exchange_strong(curr_status,2);
               return -1;
            }
            else
                find_key = local_tail->vnode->val;
            return 1;
        }

        int addL(Linked_List<key_type, Vnode<val_type> *> *lln, aimodel_status *aim_stat, Snap_Vnode *&local_tail, val_type &find_key,fstream * logfile, bool debug )
        {    
            ll_Node<key_type, Vnode<val_type> *> *curr_n = (ll_Node<key_type, Vnode<val_type> *>*)unset_freeze_mark((uintptr_t)lln->head->next.load());

            while (curr_n->key != std::numeric_limits<key_type>::max())
            {
                while (is_marked_ref((long)curr_n->value->vnext.load()) and curr_n->key != std::numeric_limits<key_type>::max())
                    curr_n = (ll_Node<key_type, Vnode<val_type> *>*)unset_freeze_mark((uintptr_t)curr_n->next.load());
                if (curr_n->key > find_key)
                {
                    Snap_Vnode *snap_vnode = new Snap_Vnode(curr_n->value, end_snap_Vnode);
                    Snap_Vnode *tmp_head = nullptr;
                    bool head_up = false , tail_up = false;
                    if (local_tail == nullptr) {
                        head_up = aim_stat->head.compare_exchange_strong(tmp_head, snap_vnode);
                    }
                    while (find_key < curr_n->key)
                    {
                        Snap_Vnode * tmp = end_snap_Vnode;
                        if (local_tail != nullptr) {
                            tail_up = local_tail->vnext.compare_exchange_strong(tmp, snap_vnode);

                            if(tail_up){
                                Snap_Vnode *tmp_snap_Vnode = local_tail;
                                aim_stat->tail.compare_exchange_strong(tmp_snap_Vnode, snap_vnode);

                            }
                        }
                        else{
                            //This part will run only once given an aimodel
                            Snap_Vnode *tmp_snap_Vnode = local_tail;
                            if(head_up)
                                tail_up = aim_stat->tail.compare_exchange_strong(tmp_snap_Vnode, snap_vnode);
                            else
                                tail_up = aim_stat->tail.compare_exchange_strong(tmp_snap_Vnode , tmp_head);

                        }



                        int ret = update_local_tail(aim_stat, local_tail, find_key);
                        if (ret== -1) {
                            if (!head_up and !tail_up)
                                delete snap_vnode;
                            return -1;
                        }
                    }
                    if(!head_up and !tail_up)
                        delete snap_vnode;
                }
                curr_n = (ll_Node<key_type, Vnode<val_type> *>*)unset_freeze_mark((uintptr_t)curr_n->next.load());
            }
            return 1;
        }

        int process_mobs(struct model_or_bin<key_type,Vnode<val_type> *>   *mobs_lflb, aimodel_status *aim_stat, Snap_Vnode *&local_tail, val_type &find_key,
                fstream * logfile, bool debug )
        {
            int ret = 1;
            if (aim_stat->status.load() == 2)
                return -1;
            
            if (mobs_lflb->isbin){
                //type node
                auto *current_n = (Node < key_type, Vnode<val_type> *> *)get_unmarked_ref((long)mobs_lflb->mob.lflb->root.load());

                if (current_n->isinternal()){
                    auto *current_root = (Internal_Node<key_type, Vnode<val_type> *> *)current_n;
                    key_type ptr_idx = std::lower_bound(current_root -> key.begin(), current_root -> key.begin() + current_root -> count, find_key) - current_root -> key.begin();
                    long cnt = current_root->count.load();
                    for(key_type i=ptr_idx; i<cnt-1; i++){
                        if(((Internal_Node<key_type, Vnode<val_type> *>*)current_root)->key[i] < find_key)
                            continue;
                        
                        leaf_node<key_type, Vnode<val_type> *> *current_leaf = ((Internal_Node<key_type, Vnode<val_type> *>*)current_root)->ptr[i];
                        Linked_List<key_type, Vnode<val_type> *> *lln = &current_leaf->data_array_list;
                        ret = addL(lln, aim_stat, local_tail, find_key, logfile, debug);
                        if(ret == -1)
                            return -1;
                    }
                    leaf_node<key_type, Vnode<val_type> *> *current_leaf = ((Internal_Node<key_type, Vnode<val_type> *>*)current_root)->ptr[cnt-1];
                    Linked_List<key_type, Vnode<val_type> *> *lln = &current_leaf->data_array_list;
                    ret = addL(lln, aim_stat, local_tail, find_key, logfile, debug);
                    if (ret == -1)
                        return -1;
                }
                else{

                    Linked_List<key_type, Vnode<val_type> *> *lln = &((leaf_node<key_type, Vnode<val_type> *>*)current_n)->data_array_list;
                    ret = addL(lln, aim_stat, local_tail, find_key, logfile, debug);
                    if (ret == -1)
                        return -1;
                }
            }
            else{
                KanvaModel<key_type, Vnode<val_type> *> *ai = mobs_lflb->mob.ai;
                ret = kanva_model_iterator(ai, aim_stat, local_tail, find_key, false, logfile, debug);
                if(ret == -1)
                    return -1;  
            }
            return ret;
        }
            
        int kanva_model_iterator(KanvaModel<key_type, Vnode<val_type> *> *aimodel, aimodel_status *aim_stat, Snap_Vnode *&local_tail, val_type &find_key, bool is_first ,
                                 fstream * logfile, bool debug )
        {

            int ret = 1;
            if(aim_stat->status.load() == 2)
                return -1;

            size_t pos = aimodel->predict(find_key);
            pos = aimodel->locate_in_levelbin(find_key, pos);

            key_type bin_pos = find_key<aimodel->get_keys(pos)?pos:(pos+1);

            for (key_type i = bin_pos; i < aimodel->get_capacity(); i++)
            {
                model_or_bin<key_type, Vnode<val_type>*> *mobs = aimodel->get_mobs(i);
                if (mobs != nullptr and aimodel->get_keys(i) > find_key)
                {
                    ret = process_mobs(aimodel->get_mobs(i), aim_stat, local_tail, find_key,logfile , debug);
                    if(ret == -1)
                        return -1;  
                }
                Vnode<val_type> *v = aimodel->get_vals(i);

                if(!is_marked_ref((long)v->vnext.load()) and v->val > find_key)
                {
                    Snap_Vnode *snap_vnode = new Snap_Vnode(v, end_snap_Vnode);
                    bool head_up = false, tail_up = false;
                    Snap_Vnode *tmp_head = nullptr;
                    if(local_tail == nullptr) {
                        //if head has been updated it will store the value
                        head_up = aim_stat->head.compare_exchange_strong(tmp_head, snap_vnode);
                    }
                    while(find_key  < v -> val){
                        //update local_tail next
                        Snap_Vnode * tmp = end_snap_Vnode;
                        if (local_tail != nullptr) {
                            tail_up = local_tail->vnext.compare_exchange_strong(tmp, snap_vnode);

                            if(tail_up) {
                                Snap_Vnode *tmp_tail = local_tail;
                                aim_stat->tail.compare_exchange_strong(tmp_tail, snap_vnode);

                            }
                        }
                        else{
                            Snap_Vnode *tmp_tail = nullptr;
                            if(head_up)
                                tail_up = aim_stat->tail.compare_exchange_strong(tmp_tail, snap_vnode);
                            else
                                tail_up = aim_stat->tail.compare_exchange_strong(tmp_tail , tmp_head);

                        }




                        //helping
                        ret = update_local_tail(aim_stat, local_tail, find_key);
                        if(ret == -1){
                            if (!head_up and !tail_up)
                                delete snap_vnode;
                            return -1;
                        }
                    }
                    if (!head_up and !tail_up)
                        delete snap_vnode;
                }
            }

            //check last bin
            if (aimodel->get_mobs(aimodel->get_capacity()) != nullptr)
            {
                ret = process_mobs(aimodel->get_mobs(aimodel->get_capacity()), aim_stat, local_tail, find_key, logfile, debug);
                if(ret == -1)
                    return -1;
            }

            if(is_first){
                Snap_Vnode *tmp_head = end_snap_Vnode;
                local_tail = aim_stat->tail.load();
//                std:: cout << local_tail << std::endl;
                if (local_tail != nullptr) {
                    while (!local_tail->vnext.compare_exchange_strong(tmp_head, (Snap_Vnode *) get_marked_ref(
                            (long) end_snap_Vnode))) {
                        if (is_marked_ref((long) tmp_head))
                            break;
                        local_tail = tmp_head;
                        tmp_head = end_snap_Vnode;
                    }

                }
                int curr_status = 1;
                aim_stat->status.compare_exchange_strong(curr_status, 2);
            }
            return ret;
        }
        void iterator(fstream * logfile, bool debug, int thread_num)
        {

            // reading the vertex list
            vector<KanvaModel<key_type, Vnode<val_type> *> > aimodels = km->get_aimodels();
            aimodel_status  *cur_aim_stat = nullptr;
            if(this->aim_status_list == nullptr){
                aimodel_status *aim_stat_new = new aimodel_status[aimodels.size()];
                bool ret = this->aim_status_list.compare_exchange_strong(cur_aim_stat , aim_stat_new);
                if(ret)//if false cur_aim_stat will already have the correct val
                {
                    cur_aim_stat = aim_stat_new;
                }
            }
            else
            {
                cur_aim_stat = this->aim_status_list.load();
            }



            //1st iteratior
            for (int i = 0; i < aimodels.size(); i++)
            {
                if (this->read_edge)
                    break;
                aimodel_status *aim_stat = &cur_aim_stat[i];
                Snap_Vnode *local_tail = nullptr;
                val_type find_key = 0;
                int curr_status = 0;
                if(aim_stat->status.compare_exchange_strong(curr_status, 1))
                    kanva_model_iterator(&aimodels[i], aim_stat, local_tail, find_key, true, logfile, debug);


            }

            //2nd iterator

            for(int i = 0; i < aimodels.size(); i++)
            {
                if(this->read_edge)
                    break;
                aimodel_status *aim_stat = &cur_aim_stat[i];

                if(aim_stat->status.load() != 2){
                    Snap_Vnode *local_tail = aim_stat->tail;
                    val_type  find_key = 0;
                    if(local_tail != nullptr)
                        find_key = local_tail->vnode->val;
                    int curr_status = 0;
                    aim_stat->status.compare_exchange_strong(curr_status, 1);
                    if (curr_status !=2)//gets upddated if cas fails above
                        kanva_model_iterator(&aimodels[i], aim_stat, local_tail, find_key, true, logfile,debug);
                }
            }



            Snap_Vnode * snap_vertex_ptr = this->head_snap_Vnode;
//            Snap_Vnode * test

            for(int i = 0; i < aimodels.size(); i++)
            {
                if(aim_status_list[i].head.load() != nullptr) {
                    Snap_Vnode * tmp_endn = (Snap_Vnode *)get_marked_ref((long) end_snap_Vnode);
                    snap_vertex_ptr->vnext.compare_exchange_strong( tmp_endn,  aim_status_list[i].head.load());

                    snap_vertex_ptr = aim_status_list[i].tail.load();

                }
            }





            this->read_edge = true;

            Snap_Vnode *snap_edge_vertex_ptr = head_snap_Vnode->vnext;// used to identify current vertex we are iterating
            //iterate through the edge
            ///ist iteration

            long counter = 0;
            long loc_vertex_iter_counter;


            while (!this->iteration_completed){

                loc_vertex_iter_counter = this->vertex_iter_counter++;

                while(!is_marked_ref((long)snap_edge_vertex_ptr) and counter < loc_vertex_iter_counter){
                    snap_edge_vertex_ptr = snap_edge_vertex_ptr->vnext;
                    counter++;
                }

                //reached end of vertex list
                if(is_marked_ref((long)snap_edge_vertex_ptr))
                    break;

                int tmp = 0;
                if(atomic_compare_exchange_strong(&snap_edge_vertex_ptr->iter_edge_status , &tmp , 1)){
                    Snap_Enode *curr_snap_Enode = snap_edge_vertex_ptr->ehead;//next of ehead will never me marked
                    Snap_Enode *next_snap_Enode = curr_snap_Enode->enext.load();
                    while(get_unmarked_ref((long)next_snap_Enode) != (long)end_snap_Enode){
                        curr_snap_Enode = next_snap_Enode ;
                        next_snap_Enode = curr_snap_Enode->enext;
                    }

                     Enode<val_type> * next_enode = (Enode<val_type> *)get_unmarked_ref((long)curr_snap_Enode->enode->enext.load()); //this will not be marked
                    while(get_unmarked_ref((long) next_enode) != (long)end_Enode_T and snap_edge_vertex_ptr->iter_edge_status != 2){
                        if(is_marked_ref((long)next_enode->enext.load())){
                            next_enode = (Enode<val_type> *)get_unmarked_ref((long) next_enode->enext.load());
                            continue;
                        }
                        Snap_Enode *snap_Enode = new Snap_Enode(next_enode , next_snap_Enode);
                        Snap_Enode * tmp_end_snap_Enode = end_snap_Enode;
                        if(!atomic_compare_exchange_strong(&curr_snap_Enode->enext , &tmp_end_snap_Enode ,snap_Enode ))
                        {
                            delete snap_Enode;
                            if(is_marked_ref((long)tmp_end_snap_Enode))//end snap enode is marked for this edge list
                                break;
                        }
                        curr_snap_Enode = curr_snap_Enode->enext.load();
                        next_enode = (Enode<val_type> *)get_unmarked_ref((long) curr_snap_Enode->enode->enext.load());

                    }

                    if(get_unmarked_ref((long) next_enode) == (long)end_Enode_T and snap_edge_vertex_ptr->iter_edge_status != 2){
                        Snap_Enode * tmp_end_snap_Enode = end_snap_Enode;
                        while(!atomic_compare_exchange_strong(&curr_snap_Enode->enext , &tmp_end_snap_Enode , (Snap_Enode*)set_mark((long)end_snap_Enode)) ) //either some thread has updated to marked(end_snap_enode) or added another edge
                        {
                            if(is_marked_ref((long)curr_snap_Enode->enext.load()))
                                break;

                            curr_snap_Enode = curr_snap_Enode->enext;
                        }
                    }

                    tmp = 1;
                    atomic_compare_exchange_strong(&snap_edge_vertex_ptr->iter_edge_status , &tmp , 2);


                }
            }

            //2nd iteration
            snap_edge_vertex_ptr = head_snap_Vnode->vnext;
            int tmp;

            while (!is_marked_ref((long)snap_edge_vertex_ptr) and !this->iteration_completed){
                if(snap_edge_vertex_ptr->iter_edge_status != 2){
                    tmp = 0;
                    //if node is
                    if(!atomic_compare_exchange_strong(&snap_edge_vertex_ptr->iter_edge_status , &tmp , 1)){
                        if(tmp == 2){
                            //some other node has already completed
                            snap_edge_vertex_ptr = snap_edge_vertex_ptr->vnext;
                            continue;
                        }

                    }

                    Snap_Enode *curr_snap_Enode = snap_edge_vertex_ptr->ehead;//next of ehead will never me marked
                    Snap_Enode *next_snap_Enode = curr_snap_Enode->enext.load();
                    while(get_unmarked_ref((long)next_snap_Enode) != (long)end_snap_Enode){
                        curr_snap_Enode = next_snap_Enode ;
                        next_snap_Enode = curr_snap_Enode->enext;
                    }
                    if(debug)
                        *logfile << "curr_snap_Enode->enode " << curr_snap_Enode->enode  << "snap_edge_vertex_ptr val" << snap_edge_vertex_ptr->vnode->val<< endl;
                    Enode<val_type> * next_enode = (Enode<val_type> *)get_unmarked_ref((long)curr_snap_Enode->enode->enext.load()); //this will not be marked
                    while(get_unmarked_ref((long) next_enode) != (long)end_Enode_T and snap_edge_vertex_ptr->iter_edge_status != 2){
                        if(is_marked_ref((long)next_enode->enext.load())){
                            next_enode = (Enode<val_type> *)get_unmarked_ref((long) next_enode->enext.load());
                            continue;
                        }
                        Snap_Enode *snap_Enode = new Snap_Enode(next_enode , next_snap_Enode);
                        Snap_Enode * tmp_end_snap_Enode = end_snap_Enode;
                        if(!atomic_compare_exchange_strong(&curr_snap_Enode->enext , &tmp_end_snap_Enode ,snap_Enode ))
                        {
                            delete snap_Enode;
                            if(is_marked_ref((long)tmp_end_snap_Enode))//end snap enode is marked for this edge list
                                break;
                        }
                        curr_snap_Enode = curr_snap_Enode->enext.load();
                        next_enode = (Enode<val_type> *)get_unmarked_ref((long) curr_snap_Enode->enode->enext.load());

                    }

                    if(get_unmarked_ref((long) next_enode) == (long)end_Enode_T and snap_edge_vertex_ptr->iter_edge_status != 2){
                        Snap_Enode * tmp_end_snap_Enode = end_snap_Enode;
                        while(!atomic_compare_exchange_strong(&curr_snap_Enode->enext , &tmp_end_snap_Enode , (Snap_Enode*)set_mark((long)end_snap_Enode)) ) //either some thread has updated to marked(end_snap_enode) or added another edge
                        {
                            if(is_marked_ref((long)curr_snap_Enode->enext.load()))
                                break;

                            curr_snap_Enode = curr_snap_Enode->enext;
                        }
                    }
                    tmp = 1;
                    atomic_compare_exchange_strong(&snap_edge_vertex_ptr->iter_edge_status , &tmp , 2);
                }
                snap_edge_vertex_ptr = snap_edge_vertex_ptr ->vnext;
            }



            this->iteration_completed = true;
        }

        /**
         * @brief This method adds block nodes at the head of each threads edge report and vertex report linked list
         * 
         * @return ** void 
         */

        void blockFurtherReports(fstream * logfile, bool debug){
            //the head of all the threads vertex reports and edge reports will be assigned to following block reports
            //block report action = 3
            if(debug)
                (*logfile) << "Blocking reports" << endl;
            for(int i=0;i<this->no_of_threads;i++)
            {
                //block vertex report of thread i
                
                VertexReport* vrep_head = this->reports[i]->head_vertex_report;
                VertexReport * block_vreport = new VertexReport(nullptr  , 3 , vrep_head);

                if(vrep_head == nullptr || vrep_head->action != 3){
                    // since will only run limited numbner of times, therefore WF
                    while( !atomic_compare_exchange_strong(&reports[i]->head_vertex_report, &vrep_head, block_vreport))
                    {
                        vrep_head = this->reports[i]->head_vertex_report;
                        if(vrep_head->action == 3)
                        {
                            delete block_vreport;
                            break;
                        }
                        block_vreport->nextReport = vrep_head;
                    }
                }
                if(debug)
                    (*logfile) << "Blocked vertex reports" << endl;
                //block edge report of thread i
                
                EdgeReport* erep_head = this->reports[i]->head_edge_report;
                EdgeReport * block_ereport = new EdgeReport(nullptr , nullptr ,3, erep_head );

                 // since will only run limited numbner of times, therefore WF
                 if(erep_head== nullptr || erep_head->action!= 3 ){
                    while(!atomic_compare_exchange_strong(&reports[i]->head_edge_report, &erep_head, block_ereport))
                    {
                        erep_head = this->reports[i]->head_edge_report;
                        if(erep_head->action == 3){
                            delete block_ereport ;
                            break;
                        }
                        block_ereport->nextReport = erep_head;
                    }
                 }
                if(debug)
                    (*logfile) << "Blocked edge reports" << endl;

            }
        }

        void reconstructUsingReports(fstream * logfile , bool debug){
            Snap_Vnode *next_V = head_snap_Vnode;
           
            vector<VertexReport> *vreports  = sorted_vertex_reports_ptr.load();
            if(vreports == nullptr){
                vector<VertexReport> *vreports1  = new vector<VertexReport>() ;
                for (int i = 0; i < no_of_threads; i++)
                {
                    VertexReport *curr_head= reports[i]->head_vertex_report;
                    curr_head = curr_head->nextReport; // ignore the dummy report
                    while(curr_head)
                    {
                        if(debug)
                            (*logfile)<< "Vertex add report " << curr_head->vnode->val <<"("<< curr_head->vnode<<") " << " action " << curr_head->action << endl; 
                        vreports1->push_back(*curr_head);
                        curr_head = curr_head->nextReport;
                        
                    }
                }

                vreports = new vector<VertexReport>(vreports1->size());
                partial_sort_copy(vreports1->begin(), vreports1->end(),vreports->begin(), vreports->end() ,&vertex_comparator);
                delete vreports1;
                vector<VertexReport> *tmp = nullptr;
                if(!atomic_compare_exchange_strong(&sorted_vertex_reports_ptr , &tmp , vreports)){
                    delete vreports;
                    vreports = tmp; //if cas fails tmp will have the current value
                }
            }
            
            
           
            
            Snap_Vnode *prev_snap_Vnode = head_snap_Vnode; 
            Snap_Vnode *next_snap_Vnode = prev_snap_Vnode->vnext;
            
            long vreport_size = vreports->size();
            while (true){
                long index = vertex_report_index;
                if(index >= vreport_size)
                    break;
                long prev_index = index;
                VertexReport report = vreports->at(index);
                if(debug)
                    (*logfile) << "Vertex report : " << report.vnode->val << " action : " << report.action << endl;
                while ( (long)next_snap_Vnode != get_marked_ref((long)end_snap_Vnode) and next_snap_Vnode->vnode->val < report.vnode->val){
                    prev_snap_Vnode = next_snap_Vnode;
                    next_snap_Vnode = next_snap_Vnode->vnext;
                } 
                   
                if (report.action == 2){//insert report
                    if( (long)next_snap_Vnode == get_marked_ref((long)end_snap_Vnode)|| next_snap_Vnode->vnode != report.vnode){
                         
                        Snap_Vnode *s_vnode = new Snap_Vnode(report.vnode, next_snap_Vnode,true);

                        if(!atomic_compare_exchange_strong(&prev_snap_Vnode->vnext , &next_snap_Vnode , s_vnode)){
                            delete s_vnode;
                        }
                        if(debug)
                            (*logfile) << "Vertex Added : " <<  prev_snap_Vnode->vnext.load()->vnode->val <<"(" <<  prev_snap_Vnode->vnext.load()->vnode << ")  " << report.action << endl;
                        next_snap_Vnode = prev_snap_Vnode->vnext ;
                       
                    }
                    else
                    {
                        if(debug)
                            (*logfile) << "Vertex already present : " <<  report.vnode->val <<"(" <<  report.vnode << ")  " << report.action << endl;
                    }
                    index++;
                   
                }
                else 
                {
                    //vertex deletion

                    if (next_snap_Vnode->vnode == report.vnode) {
                        Snap_Vnode * tmp_snap_Vnode = next_snap_Vnode;
                        if(atomic_compare_exchange_strong(&prev_snap_Vnode->vnext , &tmp_snap_Vnode , next_snap_Vnode->vnext.load() )){
                            if(debug)
                                (*logfile) << "Vertex deleted : " <<  next_snap_Vnode->vnode->val <<"(" <<  next_snap_Vnode->vnode << ")  " << report.action << endl;
                        }
                        next_snap_Vnode = next_snap_Vnode ->vnext;
                    }
                    else{
                        if(debug)
                            (*logfile) << "Vertex not found : " <<  report.vnode->val <<"(" <<  report.vnode << ")  " << report.action << endl;
                    }
      
                    
                    index++;
                    while(index < vreport_size and report.vnode == vreports->at(index).vnode){
                        index++;
                    }
                    
                    
                }
                atomic_compare_exchange_strong(&vertex_report_index , &prev_index, index); //update vertex index
            }

            
            if(debug)
                (*logfile) << "Edge Iterations started" << endl;

            /// @brief processing edge reports
            vector<EdgeReport> *edge_reports = sorted_edge_reports_ptr.load();
            
            if (edge_reports == nullptr){
                edge_reports = new vector<EdgeReport>();
                for (int i = 0; i < no_of_threads; i++)
                {
                    EdgeReport *curr_head= reports[i]->head_edge_report;
                    curr_head = curr_head->nextReport; // ignore the dummy report
                    while(curr_head)
                    {
                        
                        edge_reports->push_back(*curr_head);
                        curr_head = curr_head->nextReport;
                    }
                }
                 
                sort(edge_reports->begin(), edge_reports->end(), &edge_comparator);
                vector<EdgeReport> * tmp = nullptr;
                if(!atomic_compare_exchange_strong(&sorted_edge_reports_ptr , &tmp , edge_reports)){            
                    delete edge_reports;
                    edge_reports = tmp;//cas fails tmp will have the current value
                }
                
            }
            Snap_Vnode * loc_snap_vertex_ptr = head_snap_Vnode->vnext;
            long loc_report_index = 0;
             if(debug)
                *logfile << "Ist Iteration "<< endl; 
            long ereport_size = edge_reports->size();
            long counter = 0;
            long loc_vertex_reconstruct_counter;
            //ist iteration
            while( !this->reconstruction_completed){
                int tmp = 0;    
                long prev_index = -1;
                loc_vertex_reconstruct_counter = this->vertex_reconstruct_counter++;

                while(!is_marked_ref((long)loc_snap_vertex_ptr) and counter < loc_vertex_reconstruct_counter){
                    loc_snap_vertex_ptr = loc_snap_vertex_ptr->vnext;
                    counter++;
                }

                //reached end of vertex list
                if(is_marked_ref((long)loc_snap_vertex_ptr))
                    break;

                if(atomic_compare_exchange_strong(&loc_snap_vertex_ptr->edge_status , &tmp , 1)){
                    if(debug)
                        *logfile << "Processing node " << loc_snap_vertex_ptr->vnode->val << "(" <<loc_snap_vertex_ptr->vnode<< ")" << endl; 
                    //edges of this nodes isnt processed by any thread
                    //update the dest_vnode ptr to next of head snap_vnode

                    //fetch the report with source is the cur snap vertex ptr
                    while(loc_report_index < edge_reports->size() and edge_reports->at(loc_report_index).source->val < loc_snap_vertex_ptr->vnode->val){
                        loc_report_index++;
                    }
                    Snap_Enode * prev_snap_edge = loc_snap_vertex_ptr->ehead;
                    Snap_Enode * curr_snap_edge = loc_snap_vertex_ptr->ehead->enext;
                    Snap_Vnode * dest_vsnap_ptr = head_snap_Vnode->vnext;
                    if(loc_report_index == edge_reports->size() || edge_reports->at(loc_report_index).source !=loc_snap_vertex_ptr->vnode )
                    {
                        //no report exist for the given source...
                        //store -2 in snap_vnodes edge report to indicate no reports for edges of this snap vnode
                        atomic_compare_exchange_strong(&loc_snap_vertex_ptr->report_index , &prev_index , -2L);//if fails some other thread
                        //would have done it in its 2nd iteration through the snap vnodes

                    }
                    else
                    {
                        

                        if(!atomic_compare_exchange_strong(&loc_snap_vertex_ptr->report_index , &prev_index , loc_report_index))
                            loc_report_index = prev_index;
                        else
                            prev_index = loc_report_index;

                        
                        // report found with same source
                        
                        while(loc_report_index < ereport_size and edge_reports->at(loc_report_index).source ==loc_snap_vertex_ptr->vnode )
                        {
                            EdgeReport curr_ereport = edge_reports->at(loc_report_index);
                            //if some thread has completed processing the edge list
                            if(loc_snap_vertex_ptr->edge_status == 2)
                                break;

                            //verify and update all the edges till the current report edge
                            
                            while ((long)curr_snap_edge != get_marked_ref((long)end_snap_Enode) and curr_snap_edge->enode->val < curr_ereport.enode->val){
                                if(curr_snap_edge->d_vnode != nullptr){
                                    prev_snap_edge = curr_snap_edge;
                                    curr_snap_edge = curr_snap_edge->enext;
                                    continue;
                                }
                                //check if dest vertex is present for curr snap edge(not the report related edge)
                                while((long)dest_vsnap_ptr != get_marked_ref((long)end_snap_Vnode) 
                                            and dest_vsnap_ptr->vnode->val < curr_snap_edge->enode->val){
                                    dest_vsnap_ptr = dest_vsnap_ptr->vnext;
                                }
                                if ((long)dest_vsnap_ptr == get_marked_ref((long)end_snap_Vnode) || dest_vsnap_ptr->vnode != curr_snap_edge->enode->v_dest){
                                    //delete the edge
                                    Snap_Enode * tmp_snap_edge =  curr_snap_edge;
                                    atomic_compare_exchange_strong(&prev_snap_edge->enext , &tmp_snap_edge , curr_snap_edge->enext.load());
                                    curr_snap_edge = prev_snap_edge->enext;
                                }
                                else{
                                
                                    Snap_Vnode * tmp = nullptr;
                                    atomic_compare_exchange_strong(&curr_snap_edge->d_vnode , &tmp, dest_vsnap_ptr);
                                    //curr_snap_edge->d_vnode = dest_vsnap_ptr;
                                    prev_snap_edge = curr_snap_edge;
                                    curr_snap_edge = curr_snap_edge->enext;
                                }
                            }

                            //if snap edges reach end of the list
                            if((long)dest_vsnap_ptr != get_marked_ref((long)end_snap_Vnode))
                                break;

                            if(curr_ereport.action == 1){
                                //delete report
                                //if edge is present delete the edge
                                if (curr_snap_edge->enode == curr_ereport.enode){
                                    atomic_compare_exchange_strong(&prev_snap_edge->enext ,&curr_snap_edge ,curr_snap_edge->enext.load());
                                    curr_snap_edge = prev_snap_edge->enext;
                                }
                            
                            }
                            else{
                                //insert report
                                //if edge is present ignore
                                //else verify if the dest_v's snap vnode exists and add the new snap enode
                                while ((long)dest_vsnap_ptr != get_marked_ref((long)end_snap_Vnode) 
                                        and dest_vsnap_ptr->vnode->val < curr_ereport.enode->v_dest->val){
                                    dest_vsnap_ptr = dest_vsnap_ptr->vnext;
                                }
                                if(curr_snap_edge->enode != curr_ereport.enode){
                                    if ((long)dest_vsnap_ptr != get_marked_ref((long)end_snap_Vnode) && dest_vsnap_ptr->vnode == curr_ereport.enode->v_dest){ //no delete report TO edge address and value
                                                    
                                        Snap_Enode *snode = new Snap_Enode(curr_ereport.enode,curr_snap_edge, dest_vsnap_ptr);
                                        if(!atomic_compare_exchange_strong(&prev_snap_edge->enext ,&curr_snap_edge ,snode))
                                            delete snode;
                                        prev_snap_edge = prev_snap_edge->enext;
                                        
                                    }

                                }
                                else
                                {
                                    //if edge snap node already exists and vdest doest
                                    if(dest_vsnap_ptr->vnode != curr_ereport.enode->v_dest){
                                        //detete the snap edge
                                        atomic_compare_exchange_strong(&prev_snap_edge->enext ,&curr_snap_edge ,curr_snap_edge->enext.load());
                                        curr_snap_edge = prev_snap_edge->enext;
                                    }
                                }
                            }

                            //ignore all the insert report belonging to the same edge and source
                            //update the report index for the given node

                            loc_report_index++;
                            EdgeReport tmp_rep = edge_reports->at(loc_report_index);
                            while( loc_report_index < ereport_size and tmp_rep.enode == curr_ereport.enode and  tmp_rep.source == loc_snap_vertex_ptr->vnode )//ignore all report belonging to edge
                                loc_report_index++;
                            if(atomic_compare_exchange_strong(&loc_snap_vertex_ptr->report_index,&prev_index,loc_report_index))
                                prev_index = loc_report_index;
                            else
                                loc_report_index = prev_index;
                        }
                        
                        
                    }


                   //vertify and update the destination pointer for each vertex; check if status is 2 
                    //while(snapedge is marked)
                    while ((long)curr_snap_edge != get_marked_ref((long)end_snap_Enode)){

                        if(loc_snap_vertex_ptr->edge_status == 2)
                                break;

                        if(curr_snap_edge->d_vnode != nullptr){
                            prev_snap_edge = curr_snap_edge;
                            curr_snap_edge = curr_snap_edge->enext;
                            continue;
                        }

                        //check if dest vertex is present for curr snap edge(not the report related edge)
                        while((long)dest_vsnap_ptr != get_marked_ref((long)end_snap_Vnode)
                                    and dest_vsnap_ptr->vnode->val <
                                    curr_snap_edge->enode->val){
                            dest_vsnap_ptr = dest_vsnap_ptr->vnext;
                        }
                        if ((long)dest_vsnap_ptr == get_marked_ref((long)end_snap_Vnode) || dest_vsnap_ptr->vnode != curr_snap_edge->enode->v_dest){
                            //delete the edge
                            Snap_Enode * tmp_snap_edge =  curr_snap_edge;
                            atomic_compare_exchange_strong(&prev_snap_edge->enext , &tmp_snap_edge , curr_snap_edge->enext.load());
                            curr_snap_edge = prev_snap_edge->enext;
                        }
                        else{

                            Snap_Vnode * tmp = nullptr;
                            atomic_compare_exchange_strong(&curr_snap_edge->d_vnode , &tmp, dest_vsnap_ptr);
                            //curr_snap_edge->d_vnode = dest_vsnap_ptr;
                            prev_snap_edge = curr_snap_edge;
                            curr_snap_edge = curr_snap_edge->enext;
                        }
                    }



                    int tmp_edge_status = 1;
                    atomic_compare_exchange_strong(&loc_snap_vertex_ptr->edge_status , &tmp_edge_status , 2);
                }

                //loc_snap_vertex_ptr = loc_snap_vertex_ptr->vnext;
            }

            if(debug){
                *logfile << "2nd Iteration "<< endl; 
            }
            //once a thread has iterated all the vnodes
            //2nd iteration
            
            loc_snap_vertex_ptr = head_snap_Vnode->vnext;
            loc_report_index = 0;
            
            //while the snap vertex is not marked ie. marked_end_snap_enode
            while(!is_marked_ref((long)loc_snap_vertex_ptr) and !this->reconstruction_completed)
            {
                long prev_index = -1;
                //if snap vertex edge status = 1 ie. edges are still not processed completely
                if(loc_snap_vertex_ptr->edge_status != 2){
                    int tmp = 0;
                    if(!atomic_compare_exchange_strong(&loc_snap_vertex_ptr->edge_status , &tmp , 1)){
                        if(tmp == 2){
                            //some other node has already completed
                            loc_snap_vertex_ptr = loc_snap_vertex_ptr->vnext;
                            continue;
                        }
                        
                    }

                    Snap_Enode * prev_snap_edge = loc_snap_vertex_ptr->ehead;
                    Snap_Enode * curr_snap_edge = loc_snap_vertex_ptr->ehead->enext;
                    Snap_Vnode * dest_vsnap_ptr = head_snap_Vnode->vnext;
                    
                    
                    
                    //if snap vnode report index is still -1 ...not updated by other thread

                    if(loc_snap_vertex_ptr->report_index == -1){
                        //simillar to the above code....fetch the report corresponding to cuur vnode as source of edge report
                        while(loc_report_index < edge_reports->size() and edge_reports->at(loc_report_index).source->val < loc_snap_vertex_ptr->vnode->val){
                            loc_report_index++;
                        }
                        prev_index = -1;
                        //if no such report
                        if(loc_report_index == edge_reports->size() || edge_reports->at(loc_report_index).source->val >= loc_snap_vertex_ptr->vnode->val)
                        {    
                            //mark edge report as -2
                            
                            atomic_compare_exchange_strong(&loc_snap_vertex_ptr->report_index , &prev_index , -2L);

                        }
                        else
                        {
                            //update the edge index of snap vnode with index
                            if(atomic_compare_exchange_strong(&loc_snap_vertex_ptr->report_index , &prev_index , loc_report_index))
                                prev_index = loc_report_index;
                            else
                                loc_report_index = prev_index;
                        }
                    }
                    //if edge report is not -2
                    if(loc_snap_vertex_ptr->report_index != -2L){
                        
                        if(debug)
                            *logfile << "Re-Processing node " << loc_snap_vertex_ptr->vnode->val << "(" <<loc_snap_vertex_ptr->vnode<< ")" << endl; 
                    
                        //while edge report increments to next source or end of the report list
                        while(loc_report_index < ereport_size and  edge_reports->at(loc_report_index).source ==loc_snap_vertex_ptr->vnode and loc_snap_vertex_ptr->edge_status != 2 )
                        {

                            EdgeReport curr_ereport = edge_reports->at(loc_report_index);
                            //verify and update all the edges till the current report edge
                            while ((long)curr_snap_edge != get_marked_ref((long)end_snap_Enode) and curr_snap_edge->enode->val < curr_ereport.enode->val){
                                if(curr_snap_edge->d_vnode != nullptr){
                                    prev_snap_edge = curr_snap_edge;
                                    curr_snap_edge = curr_snap_edge->enext;
                                    continue;
                                }
                                //check if dest vertex is present for curr snap edge(not the report related edge)
                                while((long)dest_vsnap_ptr != get_marked_ref((long)end_snap_Vnode) 
                                            and dest_vsnap_ptr->vnode->val < curr_snap_edge->enode->val){
                                    dest_vsnap_ptr = dest_vsnap_ptr->vnext;
                                }
                                if ((long)dest_vsnap_ptr == get_marked_ref((long)end_snap_Vnode) || dest_vsnap_ptr->vnode != curr_snap_edge->enode->v_dest){
                                    //delete the edge
                                    Snap_Enode * tmp_snap_edge =  curr_snap_edge;
                                    atomic_compare_exchange_strong(&prev_snap_edge->enext , &tmp_snap_edge , curr_snap_edge->enext.load());
                                    curr_snap_edge = prev_snap_edge->enext;
                                }
                                else{
                                
                                    Snap_Vnode * tmp = nullptr;
                                    atomic_compare_exchange_strong(&curr_snap_edge->d_vnode , &tmp, dest_vsnap_ptr);
                                    //curr_snap_edge->d_vnode = dest_vsnap_ptr;
                                    prev_snap_edge = curr_snap_edge;
                                    curr_snap_edge = curr_snap_edge->enext;
                                }
                            }

                            //if snap edges reach end of the list
                            if((long)dest_vsnap_ptr != get_marked_ref((long)end_snap_Vnode))
                                break;

                            if(curr_ereport.action == 1){
                                //delete report
                                //if edge is present delete the edge
                                if (curr_snap_edge->enode == curr_ereport.enode){
                                    atomic_compare_exchange_strong(&prev_snap_edge->enext ,&curr_snap_edge ,curr_snap_edge->enext.load());
                                    curr_snap_edge = prev_snap_edge->enext;
                                }
                            
                            }
                            else{
                                //insert report
                                //if edge is present ignore
                                //else verify if the dest_v's snap vnode exists and add the new snap enode
                                while ((long)dest_vsnap_ptr != get_marked_ref((long)end_snap_Vnode) 
                                        and dest_vsnap_ptr->vnode->val < curr_ereport.enode->v_dest->val){
                                    dest_vsnap_ptr = dest_vsnap_ptr->vnext;
                                }
                                if(curr_snap_edge->enode != curr_ereport.enode){
                                    if ((long)dest_vsnap_ptr != get_marked_ref((long)end_snap_Vnode) && dest_vsnap_ptr->vnode == curr_ereport.enode->v_dest){ //no delete report TO edge address and value
                                                    
                                        Snap_Enode *snode = new Snap_Enode(curr_ereport.enode,curr_snap_edge, dest_vsnap_ptr);
                                        if(!atomic_compare_exchange_strong(&prev_snap_edge->enext ,&curr_snap_edge ,snode))
                                            delete snode;
                                        prev_snap_edge = prev_snap_edge->enext;
                                        
                                    }

                                }
                                else
                                {
                                    //if edge snap node already exists
                                    if(dest_vsnap_ptr->vnode != curr_ereport.enode->v_dest){
                                        atomic_compare_exchange_strong(&prev_snap_edge->enext ,&curr_snap_edge ,curr_snap_edge->enext.load());
                                        curr_snap_edge = curr_snap_edge->enext;
                                    }
                                }
                            }

                            //ignore all the insert report belonging to the same edge and source
                            //update the report index for the given node

                            loc_report_index++;
                            EdgeReport tmp_rep = edge_reports->at(loc_report_index);
                            while( loc_report_index < ereport_size and tmp_rep.enode == curr_ereport.enode and  tmp_rep.source == loc_snap_vertex_ptr->vnode )//ignore all report belonging to edge
                                loc_report_index++;
                    
                            if(atomic_compare_exchange_strong(&loc_snap_vertex_ptr->report_index,&prev_index,loc_report_index))\
                                prev_index = loc_report_index;
                            else
                                loc_report_index = prev_index;

                        }
                   
                    }
                    //vertify and update the destination pointer for each vertex;  
                    //while(snapedge is marked)
                    while ((long)curr_snap_edge != get_marked_ref((long)end_snap_Enode)){

                        if(loc_snap_vertex_ptr->edge_status == 2)
                                break;

                        if(curr_snap_edge->d_vnode != nullptr){
                            prev_snap_edge = curr_snap_edge;
                            curr_snap_edge = curr_snap_edge->enext;
                            continue;
                        }
                        //check if dest vertex is present for curr snap edge(not the report related edge)
                        while((long)dest_vsnap_ptr != get_marked_ref((long)end_snap_Vnode) 
                                    and dest_vsnap_ptr->vnode->val < curr_snap_edge->enode->val){
                            dest_vsnap_ptr = dest_vsnap_ptr->vnext;
                        }
                        if ((long)dest_vsnap_ptr == get_marked_ref((long)end_snap_Vnode) || dest_vsnap_ptr->vnode != curr_snap_edge->enode->v_dest){
                            //delete the edge
                            Snap_Enode * tmp_snap_edge =  curr_snap_edge;
                            atomic_compare_exchange_strong(&prev_snap_edge->enext , &tmp_snap_edge , curr_snap_edge->enext.load());
                            curr_snap_edge = prev_snap_edge->enext;
                        }
                        else{
                        
                            Snap_Vnode * tmp = nullptr;
                            atomic_compare_exchange_strong(&curr_snap_edge->d_vnode , &tmp, dest_vsnap_ptr);
                            //curr_snap_edge->d_vnode = dest_vsnap_ptr;
                            prev_snap_edge = curr_snap_edge;
                            curr_snap_edge = curr_snap_edge->enext;
                        }
                    }

                   
                    //update the vertex edge status to 1->2
                    int tmp_edge_status = 1;
                    atomic_compare_exchange_strong(&loc_snap_vertex_ptr->edge_status , &tmp_edge_status , 2);

                    
                }

                loc_snap_vertex_ptr = loc_snap_vertex_ptr->vnext;
            }

            this->reconstruction_completed = true;
            
        }
    

        Snap_Vnode * containsSnapV(fstream * logfile, bool debug , int key){
            Snap_Vnode * snap_Vnode_ptr =  this->head_snap_Vnode->vnext;
            //only end_snap_Vnode is marked after reconstruction
            while(is_marked_ref((long)snap_Vnode_ptr) and  snap_Vnode_ptr->vnode->val < key ){
                snap_Vnode_ptr = snap_Vnode_ptr->vnext;
                
            }
            if(is_marked_ref((long)snap_Vnode_ptr) && snap_Vnode_ptr->vnode->val == key){
                //snap vnode with key is found
                return snap_Vnode_ptr;
            }
            return nullptr;
        }

      
     
        void print_snap_graph(fstream *logfile){
            (*logfile) << "Snapped Graph ---------- of snapshot : " << this  << endl;
            Snap_Vnode * snap_vnode = head_snap_Vnode->vnext;
            while(get_unmarked_ref((long)snap_vnode) != (long)end_snap_Vnode){
                string val = to_string(snap_vnode->vnode->val);
                
                (*logfile) << val << "(" << snap_vnode->vnode << ") " <<endl ;
                //(*logfile) << val << "(" << snap_vnode << "-) " <<endl ;

                Snap_Enode *snap_enode = snap_vnode->ehead->enext;
                
                while(get_unmarked_ref((long)snap_enode) != (long)end_snap_Enode){
                    
                    string e_val = to_string(snap_enode->enode->val);
                    
                    e_val = " -> " + e_val ;
                    
                    //*logfile << "d_vnode ptr " << snap_enode->d_vnode.load()  << endl;
                    //*logfile << "d_vnode is null " << (snap_enode->d_vnode== nullptr) << endl;
                    //*logfile << "snap_enode->enode ptr " << snap_enode->enode  << endl;
                    //if(snap_enode->enode->val != snap_enode->d_vnode.load()->vnode->val)
                    //    (*logfile) << e_val <<"(" << snap_enode->enode->v_dest << ") asdsadas" <<endl ;
                    //else
                    //    (*logfile) << e_val <<"(" << snap_enode->enode->v_dest << ") " <<endl ;

                    if(snap_enode->d_vnode== nullptr){
                        (*logfile) << e_val <<"(" << snap_enode->enode->v_dest << ") HoBO" <<endl ;
                    }
                    else{
                        (*logfile) << e_val <<"(" << snap_enode->enode->v_dest << ") " <<endl ;
                    }

                    //if((long)snap_enode->d_vnode.load() > 100L){
                    //    (*logfile) << e_val <<"(" << snap_enode->d_vnode << ") " <<  flush ;
                    //    //(*logfile) << flush << endl;
                    //    *logfile <<"vis : --" << snap_enode->d_vnode.load()->visitedArray <<endl ;
                    //}
                    //else{
                    //    (*logfile) << e_val <<"(" << snap_enode->d_vnode << ") "  <<endl ;
                    //}
                    snap_enode = snap_enode -> enext;
                    
                }
                if(is_marked_ref((long)snap_enode))
                    (*logfile) <<" Tail-e " <<snap_enode << "FFFFFFFFFFFFFFF" << endl;
                else
                    (*logfile) <<" Tail-e " <<snap_enode << "SSSSSSSSSSSSSSS" << endl;
                (*logfile) << endl;
                (*logfile) << "|" <<endl;
                snap_vnode = snap_vnode->vnext;
                

            }
            (*logfile) << "Tail" << endl;
            (*logfile) << "Graph(End)-------" << endl;
        }
};
/**
 * @brief  Checks whether there is active reference to snapcollector object if not creates a new snapcollector object
 * 
 * @param graph_head head of graph vertex list which we need to iterate
 * @param max_threads max number of threads that will can access/create the snapshot object
 * @return ** SnapCollector 
 */
SnapCollector *acquireSnapCollector(Vnode<val_type> *graph_head, Kanva<key_type, Vnode<val_type> *> *km, int max_threads, fstream *logfile, bool debug)
{
    SnapCollector *SC = PSC;
  
    if (SC != nullptr and SC->isActive()){
        //int num = ++SC->threads_accessing;
        return SC;
    }
    
    SnapCollector *newSC = new SnapCollector(graph_head, km, max_threads);
    
    if(!atomic_compare_exchange_strong(&PSC , &SC , newSC)){
        //if this fails some other thread has created and updated a new snapcollector
        delete newSC ;
        newSC = SC ;
        //int num = ++newSC->threads_accessing ;
        
        
    }
    
    
    return newSC;
}

SnapCollector *takeSnapshot(Kanva<key_type, Vnode<val_type> *> *km, int max_threads, fstream *logfile, bool debug, int thread_num)
{
    Vnode<val_type> *graph_head = new Vnode<val_type>(std::numeric_limits<val_type>::min(), end_Vnode_T, NULL);
    SnapCollector *SC = acquireSnapCollector(graph_head,km, max_threads, logfile, debug);
    if (debug)
        (*logfile) << "Snapshot : " << SC << endl;

    SC->iterator(logfile, debug, thread_num);
    if (debug)
        (*logfile) << "Iterator Completed" << endl;

    SC->deactivate();
    if (debug)
        (*logfile) << "Deactivated Snapshot : " << SC << " " << SC->isActive() << endl;

    SC->blockFurtherReports(logfile, debug);
    if (debug)
        (*logfile) << "Reports Blocked" << endl;
    SC->reconstructUsingReports(logfile, debug);
    if (debug)
        (*logfile) << "Reconstruction Completed" << endl;

    return SC;
}

/**
 * @brief This method is called by the graph operations to add vertex report incase of insertion or deletion 
 * of vertex

 * @param victim 
 * @param tid 
 * @param action insert->2/delete->1/block->3
 */
void reportVertex(Vnode<val_type> *victim,int tid, int action, fstream *logfile, bool debug){
//    if(debug)
//        (*logfile) << "Report vertex : " << victim->val <<" action " << action<< endl;

    SnapCollector * SC = PSC;
    if(SC != nullptr and SC->isActive()) {
        if(action == 2 && is_marked_ref((long) victim->vnext.load()))
            return;
        
        
        VertexReport *vreport_head = SC->reports[tid]->head_vertex_report;
        VertexReport *rep = new VertexReport(victim, action, vreport_head);
        
        if (vreport_head != nullptr && vreport_head->action == 3){
           return;
        }

        bool resp = atomic_compare_exchange_strong(&SC->reports[tid]->head_vertex_report, &vreport_head, rep);
//        if(resp  and debug)
//            (*logfile) << "Added Vertex Report " << victim ->val <<" action " << action << endl;
    }
}

/**
 * @brief This method is called by the graph operations to add edge report incase of insertion or deletion 
 * of edge
 * @param victim
 * @param source_enode
 * @param tid 
 * @param action insert->2/delete->1/block->3
 */
void reportEdge(Enode<val_type> *victim, Vnode <val_type> * source_enode, int tid, int action, fstream *logfile, bool debug)
{
    SnapCollector * SC = PSC;
    if(debug)
        (*logfile) << "Report edge " << source_enode->val<<" " << victim->val << " action : " << action<<endl;
    if(SC != nullptr and SC->isActive()) {
        if(action == 2 && is_marked_ref((long) victim->enext.load()))
            return;
        
        EdgeReport *vreport_head = SC->reports[tid]->head_edge_report;
        EdgeReport *rep = new EdgeReport(victim,source_enode, 2, vreport_head);
        if (vreport_head != nullptr && vreport_head->action == 3){
            return;
        }
        if(atomic_compare_exchange_strong(&SC->reports[tid]->head_edge_report, &vreport_head, rep) and debug)
            (*logfile) << "Added Edge report " << source_enode->val<<" " << victim->val << " action : " << action<<endl;
    }
}



#endif