#include <iostream>

#include <float.h>

#include <stdint.h>

#include <stdio.h>

#include <atomic>

#include <stdlib.h>

#include <limits.h>

using namespace std;

template<typename V>
class Vnode; // so that Enode can use it
// Enode structure
template<typename V>
class Enode {
    public:
        V val; // data
        Vnode<V> * v_dest; // pointer to its vertex
        atomic<Enode<V> *> enext; // pointer to the next Enode

        Enode()
        {
            val = 0;
        }


        Enode(V val , Vnode<V> * v_dest ,Enode<V> *enext ){
            this->val = val;
            this->v_dest = v_dest;
            this->enext = enext;
        }
};


// Vnode structure
template<typename V>
class Vnode {
    public:
        V val; // data
        atomic< Vnode<V> *> vnext; // pointer to the next Vnode
        atomic < Enode<V> * > ehead; // pointer to the EHead

        Vnode(){
            val = 0;
        }

        Vnode(V val, Vnode<V> *vnext, Enode<V> *ehead){
            this->val = val;

            this->vnext = vnext;


            this->ehead = ehead;
        }



};


//these values will be used to point the end of the list (vertex or edge list)
//Enode<int> * end_Enode = new Enode<int>(INT_MAX, NULL, NULL);
//Vnode<int> * end_Vnode = new Vnode<int>(INT_MAX, NULL, NULL);

Enode<uint64_t> * end_Enode_T = new Enode<uint64_t>(std::numeric_limits<uint64_t>::max(), NULL, NULL);
Vnode<uint64_t> * end_Vnode_T = new Vnode<uint64_t>(std::numeric_limits<uint64_t>::max(), NULL, NULL);


