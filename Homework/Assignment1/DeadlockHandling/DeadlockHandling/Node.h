#pragma once


template <typename R>
struct Node {		// Used for generating the list of words in a line
	R data;	// Similar structure as the one used for lists. I'll probably try to just templatize it for future homeworks
	Node *next;
	Node *prev;

	Node() {						//general constructor
		next = prev = NULL;
	}
	Node(R newVal) {			//parametered constructor
		data = newVal;
		next = prev = NULL;
	}
};