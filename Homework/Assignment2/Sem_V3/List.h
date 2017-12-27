#pragma once
#include "Node.h"

using namespace std;

template <typename T>
class List {
public:
    Node<T> *head, *last;			// used to point to the first and last words in the current list

    List() {
        head = NULL;
        last = NULL;
    }
    List(T newVal) {
        Node<T> *temp = new Node<T>(newVal);
        temp->next = NULL;
        temp->prev = NULL;
        if (head == NULL) {			// check to see if the first node exists
            head = last = temp;		// if not, head and last are set to the new node
        }
        else {
            temp->prev = last;		// otherwise, it's added to the end and last is reassigned to new node
            last->next = temp;
            last = temp;
        }
    }

    void insertAtHead(T newVal) {
        Node<T> *temp = new Node<T>(newVal);
        if (head == NULL) {
            head = temp;		// head is set to new node if list is empty
        }
        else {
            temp->next = head;	// otherwise the new node is moved to the beginning of 
            temp->prev = NULL;	// the list and head is reassigned to point to the new node
            head->prev = temp;
        }
        head = temp;
    }

    void insertAfterHead(T newVal) {	// used to add a node immediately after the head
        Node<T> *temp = new Node<T>(newVal);		// Not used in this program
        temp->next = head->next;
        temp->prev = head;
        head->next->prev = temp;
        head->next = temp;
    }

    void insertAtEnd(T newVal) {
        if (head == NULL) {					// list is empty
            head = new Node<T>(newVal);
            last = head;					// head is made into a new node and last is made equivalent to it
            return;							// exits function
        }
        Node<T> *temp = new Node<T>(newVal);
        temp->prev = last;
        temp->next = NULL;
        if (head->next == NULL) {			// checks to see if there is only one item in the list
            head->next = temp;				// Not necessary now that I think about it, but it's already finished...
            last = temp;					// last gets reassigned to the new node
        }
        else {
            last->next = temp;
            last = temp;					// last gets reassigned to the new node
        }
    }

    void insertSort(Node<T> *cur, T val) {	// builds a list and sorts it
        if (head == NULL) {					// used for building a sorted array later
            insertAtEnd(val);
        }
        else if (cur != NULL) {
            Node<T> *temp = new Node<T>(val);
            if (cur->data < val) {
                if (cur->next != NULL) {
                    if (cur->next->data < val) {
                        insertSort(cur->next, val);			// new node is still greater than all in current list
                    }
                    else {									// spot found. insert between current and next
                        temp->next = cur->next;
                        temp->prev = cur;
                        temp->next->prev = temp;
                        cur->next = temp;
                    }
                }
                else {
                    insertAtEnd(val); // Reached end of list. Insert at end.
                }
            }
            else {
                if (cur == head) {
                    insertAtHead(val); // Less than head. Insert at Head
                }
                else {							// fallback used for inserting when less than the current
                    temp->prev = cur->prev;
                    temp->next = cur;
                    temp->prev->next = temp;
                    cur->prev = temp;
                }
            }
        }
    }

    //void addNode(Node *cur, string prefix, string injection) {	// used to add a node after @prefix
    void addNode(Node<T> *cur, T prefix, T injection) {	// used to add a node after @prefix
        if (cur != NULL) {
            Node<T> *temp;
            if (prefix == "") {											// used to match the </injection> case of the insertword command
                insertAtHead(injection);
            }
            else if (cur->data == prefix) {						// if a word is matched with prefix, then 
                temp = new Node<T>(injection);							// the new node is added after			
                if (cur->next != NULL) {							// used to check if the current node is the last node // Should have just compared it with last...
                    temp->next = cur->next;
                    temp->prev = cur;
                    cur->next->prev = temp;
                    cur->next = temp;
                }
                else {												// otherwise, the current node is the last node.
                    temp->next = NULL;
                    cur->next = temp;
                    last = temp;									// last is reassigned
                }
            }
            else {
                addNode(cur->next, prefix, injection);				// no match found. loop back.
            }
        }
    }

    void deleteThisNode(Node<T> *cur) {
        if (cur == head) {
            head = cur->next;
            head->prev = NULL;
            cur->next = NULL;
            delete cur;
            cur = NULL;
        }
        else if (cur == last) {
            last = cur->prev;
            last->next = NULL;
            cur->prev = NULL;
            delete cur;
            cur = NULL;
        }
        else {
            cur->next->prev = cur->prev;
            cur->prev->next = cur->next;
            cur->next = cur->prev = NULL;
            delete cur;
            cur = NULL;
        }
    }

    void deleteNode(Node<T> *cur, T str) {		// handles all the node pointer tomfoolery
        if (cur->data == str) {
            if (cur == head) {						// checks if the matched node is the head
                cur->next->prev = NULL;
                head = cur->next;
            }
            else if (cur == last) {				// checks if the matched node is the last
                cur->prev->next = NULL;
                last = cur->prev;
            }
            else {
                cur->prev->next = cur->next;		// adjusts prev and next pointers
                cur->next->prev = cur->prev;
            }
            delete cur;								// delete allocated pointer
            cur = NULL;								// set dangling pointer to NULL;
        }
        else {
            if (cur->next != NULL) {
                deleteNode(cur->next, str);			// current node doesn't match. Loop back with the next node
            }
        }
    }

    void display(){
        display(head);
    }

    void display(Node<T> *cur) {					// prints out node's word to console then calls itself with the next node
        if (cur != NULL) {
            cout << cur->data << endl;
            display(cur->next);
        }
        else {
            cout << endl;						// once the list has finished display, a newline character is printed
        }
    }

    void destroyList(Node<T> *curHead) {		// called at the end of the program to delete allocated memory
        if (curHead != NULL) {
            head = curHead->next;				// head is set to the next node and used at the end to call the next destroyWordList()
            delete curHead;						// deletes current node
            curHead = NULL;						// sets dangling pointer to null
            destroyList(head);				// loop back with next node.
        }
    }

	Node<T> *top(){
		return last;
	}

	Node<T> *pop(){
        if (head != last) {
            Node<T> *temp = last;
            last = last->prev;
            last->next = NULL;
            delete temp;
            temp = NULL;
        }
        else {
            delete head;
            head = last = NULL;
        }
	}
};