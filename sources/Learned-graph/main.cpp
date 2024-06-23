#include <iostream>

#include <float.h>

#include <stdint.h>

#include <stdio.h>

#include <stdlib.h>

#include <pthread.h>

#include <assert.h>

#include <getopt.h>

#include <limits.h>

#include <signal.h>

#include <sys/time.h>

#include <time.h>

#include <vector>

#include <ctime> // std::time

#include <random>

#include <algorithm>

#include <math.h>

#include <time.h>

#include <fstream>

#include <iomanip>

#include <sys/time.h>

#include <atomic>

#include <list>

#include <queue>

#include <stack>

#include <random>

#include <thread>

#include <map>

#include <random>

#include <initializer_list>

#include <unistd.h>

#include <vector>
#include <set>

#include "snapcollector.h"
#include "kanva_util.h"
#include "Kanva_impl/kanva.h"
#include "Kanva_impl/kanva_impl.h"

using namespace std;

void print_graph(fstream *logfile , Vnode<int> * graph_headv);







atomic<bool> continue_exec;

/**
 * @brief paramteter that are to be passed on to the threads
 * 
 */
struct thread_args{
    Kanva<key_type ,Vnode<val_type >*>  *graph ;
    string  logfilename ;
    int thread_num;
    bool debug ;
    int max_nodes;
    int max_threads;
    double * ops;
    double * max_times;
  double * avg_times;

    vector<double> * dist_prob;

};





/**
 * @brief
 *
 * prob_arr will denote prob of different operations
 * 0->add vertex
 * 1->delete vertex
 * 2->add edge
 * 3->delete edge
 * 4->contains edge
 * 5->contains vertex
 * 6->snapshot
 *
 * @param t_args
 * @return ** void*
 */
void *thread_funct(void * t_args){

    Kanva<key_type, Vnode<val_type> *> * graph = ((struct thread_args *)t_args)->graph;
    string logFileName = ((struct thread_args *)t_args)->logfilename;
    bool debug = ((struct thread_args *)t_args)->debug;
    int thread_num = ((struct thread_args *)t_args)->thread_num;
    int max_nodes = ((struct thread_args *)t_args)->max_nodes;
    int max_threads = ((struct thread_args *)t_args)->max_threads;
    //int prob_arr[4] = ((struct thread_args *)t_args)->prob_arr;
    double * ops = ((struct thread_args *)t_args)->ops;
    double * avg_times = ((struct thread_args *)t_args)->avg_times;
    double * max_times = ((struct thread_args *)t_args)->max_times;
        vector<double> tts;//list of time taken for snapshot
    vector<double> * dist_prob = ((struct thread_args *)t_args)->dist_prob;

    //For test purpose only: making dist_prob generate only search
//    *dist_prob = {0,0,0,0,0,1,0};


    fstream logfile_th;
    if(debug){
        logFileName = logFileName + to_string(thread_num) +".txt";
        logfile_th.open(logFileName,ios::out);
    }
    random_device rd;
    mt19937 gen(rd());
    discrete_distribution<> d(dist_prob->begin() , dist_prob->end());
    int op_index;




    while(continue_exec){


        op_index = d(gen);
        switch(op_index) {
        case 0://add vertex
            {
                key_type rand_node_id = rand() % max_nodes + 1;
                if(debug)
                    logfile_th << " thread id : " << thread_num << "Add vertex  : " << rand_node_id << endl;
                chrono::high_resolution_clock::time_point startT = chrono::high_resolution_clock::now();
                Enode<key_type> * EHead = new Enode<key_type>(0, NULL,end_Enode_T);
                Vnode<val_type> *v = new Vnode<val_type>(rand_node_id, end_Vnode_T, EHead);
                bool ret = graph->insert(rand_node_id, v,thread_num);
                if(debug){
                    if(ret)
                        (logfile_th) << "Kye " << rand_node_id << " added !!" << endl;
                    else
                        (logfile_th) << "Kye " << rand_node_id << " added !!" << endl;
                }


                chrono::high_resolution_clock::time_point endT = chrono::high_resolution_clock::now();
                double timeTaken = chrono::duration_cast<chrono::microseconds>(endT-startT).count() ;

                tts.push_back(timeTaken);
                if (max_times[thread_num] < timeTaken){
                    max_times[thread_num] = timeTaken;
                }
                if(continue_exec)
                    ops[thread_num]++;
            }
            break;
        case 1:
            // delete vertex
            {
                key_type rand_node_id = rand() % max_nodes + 1;
                if(debug)
                    logfile_th << " thread id : " << thread_num << "Delete vertex : " << rand_node_id << endl;
                chrono::high_resolution_clock::time_point startT = chrono::high_resolution_clock::now();
                graph->remove(rand_node_id,thread_num);
                chrono::high_resolution_clock::time_point endT = chrono::high_resolution_clock::now();
                double timeTaken = chrono::duration_cast<chrono::microseconds>(endT-startT).count() ;

                tts.push_back(timeTaken);
                if (max_times[thread_num] < timeTaken){
                    max_times[thread_num] = timeTaken;
                }
                if(continue_exec)
                    ops[thread_num]++;
            }
            break;
        case 2:
            // add edge
            {
                key_type rand_source = rand() % max_nodes + 1;
                key_type rand_dest = rand() % max_nodes + 1;
                while(rand_dest == rand_source){
                    rand_dest = rand() % max_nodes + 1;
                }
                if(debug)
                    logfile_th << " thread id : " << thread_num << "Add edge : " << rand_source << " " << rand_dest << endl;
                chrono::high_resolution_clock::time_point startT = chrono::high_resolution_clock::now();

                graph->AddEdge(rand_source , rand_dest , thread_num,&logfile_th,debug,thread_num);
                chrono::high_resolution_clock::time_point endT = chrono::high_resolution_clock::now();
                double timeTaken = chrono::duration_cast<chrono::microseconds>(endT-startT).count() ;

                tts.push_back(timeTaken);
                if (max_times[thread_num] < timeTaken){
                    max_times[thread_num] = timeTaken;
                }
                if(continue_exec)
                    ops[thread_num]++;
            }
            break;
        case 3:
            //delete edge
            {
                key_type rand_source = rand() % max_nodes + 1;
                key_type rand_dest = rand() % max_nodes + 1;
                while(rand_dest == rand_source){
                    rand_dest = rand() % max_nodes + 1;
                }
                if(debug)
                    logfile_th << " thread id : " << thread_num << " Delete edge : " << rand_source << " " << rand_dest  << endl;
                chrono::high_resolution_clock::time_point startT = chrono::high_resolution_clock::now();
                graph->RemoveE(rand_source , rand_dest , thread_num,&logfile_th,debug,thread_num);
                chrono::high_resolution_clock::time_point endT = chrono::high_resolution_clock::now();
                double timeTaken = chrono::duration_cast<chrono::microseconds>(endT-startT).count() ;

                tts.push_back(timeTaken);
                if (max_times[thread_num] < timeTaken){
                    max_times[thread_num] = timeTaken;
                }
                if(continue_exec)
                    ops[thread_num]++;
            }
            break;
        case 4:
            //contains edge
            {
                key_type rand_source = rand() % max_nodes + 1;
                key_type rand_dest = rand() % max_nodes + 1;
                while(rand_dest == rand_source){
                    rand_dest = rand() % max_nodes + 1;
                }
                if(debug)
                    logfile_th << " thread id : " << thread_num << " Contians Edge : " << rand_source << " " << rand_dest  << endl;
                chrono::high_resolution_clock::time_point startT = chrono::high_resolution_clock::now();
                graph->ContainsE(rand_source , rand_dest , thread_num,&logfile_th,debug,thread_num);
                chrono::high_resolution_clock::time_point endT = chrono::high_resolution_clock::now();
                double timeTaken = chrono::duration_cast<chrono::microseconds>(endT-startT).count() ;

                tts.push_back(timeTaken);
                if (max_times[thread_num] < timeTaken){
                    max_times[thread_num] = timeTaken;
                }
                if(continue_exec)
                    ops[thread_num]++;
            }
            break;
        case 5:
            //contains vertex
            {
                key_type node_id = rand() % max_nodes + 1;

                if(debug)
                    logfile_th << " thread id : " << thread_num << " Contains vertex : " << node_id  << endl;
                chrono::high_resolution_clock::time_point startT = chrono::high_resolution_clock::now();
                Vnode<val_type> * node = nullptr;
                graph->find(node_id , node,thread_num);
                chrono::high_resolution_clock::time_point endT = chrono::high_resolution_clock::now();
                double timeTaken = chrono::duration_cast<chrono::microseconds>(endT-startT).count() ;

                tts.push_back(timeTaken);
                if (max_times[thread_num] < timeTaken){
                    max_times[thread_num] = timeTaken;
                }
                if(continue_exec)
                    ops[thread_num]++;
            }
        break;
        case 6:
        //    //snapshot
            {
                chrono::high_resolution_clock::time_point startT = chrono::high_resolution_clock::now();
                //print_graph(&logfile_th , graph->head);
                SnapCollector *sc = takeSnapshot(graph, max_threads, &logfile_th, debug, thread_num);

//                cout <<"Thread id : " << thread_num << " Snap taken " << endl;
                chrono::high_resolution_clock::time_point endT = chrono::high_resolution_clock::now();
                double timeTaken = chrono::duration_cast<chrono::microseconds>(endT-startT).count() ;

                tts.push_back(timeTaken);
                if (max_times[thread_num] < timeTaken){
                    max_times[thread_num] = timeTaken;
                }

                if(debug){

                    sc->print_snap_graph(&logfile_th);
                }


            }
            break;


        }
    }

    //calculate average of all the timetaken
    if(tts.size() > 0){
        double total_tts = 0;
        for(double tt : tts){
            total_tts += tt;
        }
        avg_times[thread_num] = total_tts / tts.size();

    }

    if(debug)
        logfile_th.close();


    return nullptr;

}


typedef struct init_graph_thread_args{
//    GraphList *graph ;
    bool debug ;
    bool *add_nodes;
    int max_vertices;
    int thread_num ;

}init_gph_t_args;


atomic<int> init_nodes;
atomic<int> init_edges;

////This method was created to initialize the graph with some nodes and edges
//void *init_graph_thread_funct(void * t_args){
//
////    GraphList *graph = ((init_graph_thread_args *)t_args)->graph;
//    bool debug =((init_graph_thread_args *)t_args)->debug;
//    bool  *add_nodes = ((init_graph_thread_args *)t_args)->add_nodes;
//    int thread_num = ((init_graph_thread_args *)t_args)->thread_num;
//    int max_vertices = ((init_graph_thread_args *)t_args)->max_vertices;
//
//
//    fstream logfile_th;
//    if(debug){
//        string logFileName = "../../log/log_" +  to_string(thread_num) +".txt";
//        logfile_th.open(logFileName,ios::out);
//    }
//    int remain_nodes = init_nodes ,nodes_added = 0;
//    int remain_edges = init_edges ,edges_added = 0;
//
//    while(true){
//
//
//        int op_index = *add_nodes? 0 : 1;
//        switch(op_index) {
//            case 0://add vertex
//                {
//                    int nodeid = rand() % max_vertices;
//                    bool added = graph->AddVertex(nodeid , -1 , &logfile_th , debug);
//                    while(!added){
//                        nodeid = rand() % max_vertices;
//                        added = graph->AddVertex(nodeid , -1 , &logfile_th , debug);
//                    }
//                    nodes_added++;
//                    remain_nodes = --init_nodes;
//                    if(remain_nodes <= 0){
//                        *add_nodes = false;
//                    }
//                }
//
//                break;
//            case 1:
//                // add edge
//                {
//                    int source = rand() % max_vertices;
//                    int dest = rand() % max_vertices;
//                    while(dest ==  source){
//                        dest = rand() % max_vertices;
//                    }
//                    int ret = graph->AddEdge(source ,dest , -1 , &logfile_th , debug);
//                    while(ret != 3){
//                        source = rand() % max_vertices;
//                        dest = rand() % max_vertices;
//                        while(dest ==  source){
//                            dest = rand() % max_vertices;
//                        }
//                        ret = graph->AddEdge(source ,dest , -1 , &logfile_th , debug);
//                    }
//                    edges_added++;
//                    remain_edges = --init_edges;
//                }
//                break;
//
//        }
//
//        if(remain_edges <= 0){
//            break;
//        }
//
//    }
//    if(debug)
//    {
//        logfile_th.close();
//    }
//
//
//    return nullptr;
//
//}

//
//GraphList * create_graph(int init_vertices , int init_edges){
//    GraphList * graph = new GraphList();
//    int max_vertices = 2 * init_vertices;
//
//    //add vertices
//    while(init_vertices--){
//        int nodeid = rand() % max_vertices;
//        bool ret = graph->AddVertex(nodeid , -1 , nullptr , false);
//        while(!ret){
//            nodeid = rand() % max_vertices;
//            ret = graph->AddVertex(nodeid , -1 , nullptr , false);
//        }
//    }
//
//    //add edges
//    while(init_edges--){
//        int source = rand() % max_vertices;
//        int dest = rand() % max_vertices;
//        while(dest ==  source){
//            dest = rand() % max_vertices;
//        }
//        int ret = graph->AddEdge(source ,dest , -1 , nullptr , false);
//        while(ret != 3){
//            source = rand() % max_vertices;
//            dest = rand() % max_vertices;
//            while(dest ==  source){
//                dest = rand() % max_vertices;
//            }
//            ret = graph->AddEdge(source ,dest , -1 , nullptr , false);
//        }
//    }
//    return graph;
//}



void read_from_file(string file , vector<val_type> &vertices , vector<pair<key_type, key_type>> &edges)
{
    ifstream cinn(file);
    long n,m;
    int u, v, v_;
    cinn>>n>>m;

    int i,j, e=0;

    set<key_type> vertex_set;
    int curr_size = 0;
    for(j=1; j<=m; j = j+1){
	    cinn>>u>>v;
        u = u++;
        v = v++;
        if(u <= 0 or v <= 0){
            continue;
        }
        vertex_set.insert(u);
        if(vertex_set.size() > curr_size){
            curr_size++;
            vertices.push_back(u);
        }
        vertex_set.insert(v);
        if(vertex_set.size() > curr_size){
            curr_size++;
            vertices.push_back(v);
        }

        edges.emplace_back(u,v);
      }
  //cout<<"Edge:"<<e<<endl;
}
 class GRAPH;

int main(int argc, char** argv) {
    // abc
    string logFileName = "log_";

    //will be used in script
    int num_of_threads = 2;
    int test_duration = 2;
    int initial_vertices = 10;
    int initial_edges = 2*(int)pow(10, 1);

    bool debug = false;
    vector<double> dist_prob = {1,1,1,1,1,1,1};



    if(argc > 1){
		logFileName = argv[1] ;
        num_of_threads = stoi(argv[2]) ;
        test_duration = stoi(argv[3]);
        initial_vertices = stoi(argv[4]);
        initial_edges = stoi(argv[5]);
        if(argc > 8){
            //read dist probabilities
            for(int i = 0;i< 7 ; i++){
                dist_prob[i] = stoi(argv[7+i]);
            }

        }
        if(argc >= 15){
            if(argv[14][0] == 'D'){
            	debug = true;
       		}
        }
	}
    string opFileName = logFileName + ".txt";
    fstream opfile;
    opfile.open(opFileName,ios::out);
    vector<key_type> vertices;
    vector<pair<key_type, key_type >> edges;
    read_from_file(argv[6], vertices , edges);


    //create kanva model
    Kanva<key_type ,Vnode<val_type >*> * km = create_kanva_model(vertices , edges);

    //find Node in Learned graph



//    SnapCollector *sc = takeSnapshot(km, 10, nullptr, debug, 1);

    //sc->print_snap_graph(&logfile);
    //printf(graph->ContainsE(5,4,1) != 2? "False\n" : "True\n");
    struct thread_args t_args[num_of_threads];
    pthread_t threads[num_of_threads];

    //List of throughput from each thread
    double * ops = new double[num_of_threads];

    continue_exec.store(true);
    //cout << "End snap Enode<int> " << end_snap_Enode << endl;
    //cout << "Marked End snap Enode<int> " << (Snap_Enode *)get_marked_ref((long) end_snap_Enode) << endl;
    //cout << "End snap Vnode<int> " << end_snap_Vnode << endl;
    //cout << "Marked End snap Vnode<int> " << (Snap_Vnode*) get_marked_ref((long)end_snap_Vnode) << endl;
    double * max_times = new double[num_of_threads];
    double * avg_times = new double[num_of_threads];
    for( int i=0;i < num_of_threads ;i++){


        t_args[i].logfilename = logFileName;
        t_args[i].graph = km;
         t_args[i].debug = debug;
        t_args[i].thread_num = i;
        t_args[i].max_nodes = 10 * initial_vertices;
        t_args[i].max_threads = num_of_threads;
        t_args[i].debug = debug;
        t_args[i].ops = ops;
        t_args[i].max_times = max_times;
        t_args[i].avg_times = avg_times;
        
        t_args[i].dist_prob = &dist_prob;
        pthread_create(&threads[i], NULL, thread_funct, &t_args[i]);
        
    }
    sleep(test_duration);
    continue_exec.store(false);
    for(int i=0 ; i< num_of_threads;i++){
        pthread_join(threads[i], NULL);
    }

    double max_time = 0;
    double avg_time = 0;

    for( int i = 0;i < num_of_threads ; i++){
        //check max
        if(max_time < max_times[i]){
            max_time = max_times[i];
        }
        avg_time += avg_times[i];
    }

    avg_time = avg_time / num_of_threads;


    cout << avg_time << fixed << endl;
    cout << max_time << fixed << endl;

    
    opfile.close();
    return 0;
}



/*
 * test main()
 * {
 * k /////////////////
    fstream logfile_th;
    if(debug){
        logFileName = logFileName  +".txt";
        logfile_th.open(logFileName,ios::out);
    }
    key_type dummy_key = 1;
    //Vnode<val_type> *dummy_n = nullptr;
    //Vnode<val_type> *node = km->find(dummy_key, dummy_n);
    //if (node != nullptr)
    //    cout << "Node not found in learned graph : " << node->val << endl;
    //else
    //    cout << "Node  found in learned graph" << endl;

    //// Insertion into Kanva Model
    //dummy_key = 11095;
    //Enode<key_type> *EHead = new Enode<key_type>(0, NULL, end_Enode_T);
    //Vnode<val_type> *v = new Vnode<val_type>(dummy_key, end_Vnode_T, EHead);
    //bool ret = km->insert(dummy_key, v);

    //if (ret)
    //    cout << "Node inserted into Kanva Model" << endl;

    int ret = km->remove(dummy_key);
    if (ret)
        std::cout << "Node "<< dummy_key <<" removed from Kanva Model" << std::endl;
    dummy_key = 3;
    ret = km->remove(dummy_key);
    if (ret)
        std::cout << "Node " << dummy_key << " removed from Kanva Model" << std::endl;
    dummy_key = 129;
    ret = km->remove(dummy_key);
    if (ret)
        std::cout << "Node " << dummy_key << " removed from Kanva Model" << std::endl;ret = km->remove(dummy_key);

    dummy_key = 2;
    ret = km->remove(dummy_key);
    if (ret)
        std::cout << "Node " << dummy_key << " removed from Kanva Model" << std::endl;
    else
        std::cout << "Node " << dummy_key << " not removed from Kanva Model " << std::endl;

    SnapCollector *sc = takeSnapshot(km, 10, &logfile_th, debug, 1);

    sc->print_snap_graph(&logfile_th);

    dummy_key = 100;
    Enode<key_type> *EHead = new Enode<key_type>(0, NULL, end_Enode_T);
    Vnode<val_type> *v = new Vnode<val_type>(dummy_key, end_Vnode_T, EHead);
    ret = km->insert(dummy_key , v);
    if(ret)
        cout << "Node " << dummy_key << " inserted to Kanva Model" << endl;

    dummy_key = 102;
    EHead = new Enode<key_type>(0, NULL, end_Enode_T);
    v = new Vnode<val_type>(dummy_key, end_Vnode_T, EHead);
    ret = km->insert(dummy_key , v);
    if(ret)
        cout << "Node " << dummy_key << " inserted to Kanva Model" << endl;

    int ret2 = km->AddEdge(100, 102, 1, nullptr, false);
    if (ret2 == 3)
        cout << "Edge added in 100-102 Kanva Model" << endl;

    ret2 = km->RemoveE(100, 102,1, nullptr,false);
    if(ret2 == 3)
        cout << "Edge removed from Kanva Model" << endl;

    sc = takeSnapshot(km, 10, &logfile_th, debug, 1);

    sc->print_snap_graph(&logfile_th);

    // insert edge
    //v = new Vnode<val_type>(5, end_Vnode_T, EHead);
    //ret = km->insert(5, v);
    //if (ret)
    //    cout << "Vertex " << 5 << "inserted in Kanva Model" << endl;
    //v = new Vnode<val_type>(4, end_Vnode_T, EHead);
    //ret = km->insert(4, v);
    //if (ret)
    //    cout << "Vertex " << 4 << "inserted in Kanva Model" << endl;

    //v = new Vnode<val_type>(7, end_Vnode_T, EHead);
    //ret = km->insert(7, v);
    //if (ret)
    //    cout << "Vertex " << 7 << "inserted in Kanva Model" << endl;
    //int ret2 = km->AddEdge(5, 4, 1, nullptr, false);
    //if (ret2 == 3)
    //    cout << "Edge added in 5-4 Kanva Model" << endl;
    //ret2 = km->AddEdge(5, 7, 1, nullptr, false);
    //if (ret2 == 3)
    //    cout << "Edge added in 5-7Kanva Model" << endl;
    //// search edge

    //int ret1 = km->ContainsE(5, 4, 1, nullptr, false);
    //if (ret1 == 2)
    //    cout << "Edge found in Kanva Model" << endl;

    ////    ret1 = km->RemoveE(5,4,1, nullptr,false);
    ////    if(ret1 == 3)
    ////        cout << "Edge removed from Kanva Model" << endl;

    //ret1 = km->ContainsE(5, 4, 1, nullptr, false);
    //if (ret1 == 3)
    //    cout << "Edge not found in Kanva Model" << endl;

    //////////////////

}
 */

