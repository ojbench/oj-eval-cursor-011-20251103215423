#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {
/**
 * @brief a container like std::priority_queue which is a heap internal.
 * **Exception Safety**: The `Compare` operation might throw exceptions for certain data.
 * In such cases, any ongoing operation should be terminated, and the priority queue should be restored to its original state before the operation began.
 */
template<typename T, class Compare = std::less<T>>
class priority_queue {
private:
	struct Node {
		T data;
		Node *left;
		Node *right;
		int npl;  // null path length
		
		Node(const T &val) : data(val), left(nullptr), right(nullptr), npl(0) {}
	};
	
	Node *root;
	size_t count;
	Compare cmp;
	
	// Deep copy a subtree
	Node* copyTree(Node *node) {
		if (node == nullptr) return nullptr;
		Node *newNode = new Node(node->data);
		newNode->npl = node->npl;
		try {
			newNode->left = copyTree(node->left);
			try {
				newNode->right = copyTree(node->right);
			} catch (...) {
				deleteTree(newNode->left);
				delete newNode;
				throw;
			}
		} catch (...) {
			delete newNode;
			throw;
		}
		return newNode;
	}
	
	// Delete a subtree
	void deleteTree(Node *node) {
		if (node == nullptr) return;
		deleteTree(node->left);
		deleteTree(node->right);
		delete node;
	}
	
	// Merge two leftist heaps
	Node* merge(Node *h1, Node *h2) {
		if (h1 == nullptr) return h2;
		if (h2 == nullptr) return h1;
		
		// Ensure h1 has the larger (or equal) top element
		// cmp(a, b) returns true if a < b, so if cmp(h1->data, h2->data) is true,
		// h1->data < h2->data, so we want h2 to be the root
		if (cmp(h1->data, h2->data)) {
			Node *temp = h1;
			h1 = h2;
			h2 = temp;
		}
		
		// Merge h2 with the right subtree of h1
		h1->right = merge(h1->right, h2);
		
		// Maintain leftist property
		if (h1->left == nullptr) {
			h1->left = h1->right;
			h1->right = nullptr;
		} else {
			int leftNpl = (h1->left == nullptr) ? -1 : h1->left->npl;
			int rightNpl = (h1->right == nullptr) ? -1 : h1->right->npl;
			
			if (leftNpl < rightNpl) {
				Node *temp = h1->left;
				h1->left = h1->right;
				h1->right = temp;
			}
			
			h1->npl = ((h1->right == nullptr) ? -1 : h1->right->npl) + 1;
		}
		
		return h1;
	}
	
	// Safe merge with exception handling
	Node* safeMerge(Node *h1, Node *h2) {
		if (h1 == nullptr) return h2;
		if (h2 == nullptr) return h1;
		
		try {
			// Check comparison first
			bool h1Greater = false;
			try {
				h1Greater = !cmp(h1->data, h2->data);
			} catch (...) {
				throw runtime_error();
			}
			
			if (!h1Greater) {
				Node *temp = h1;
				h1 = h2;
				h2 = temp;
			}
			
			// Merge h2 with the right subtree of h1
			h1->right = safeMerge(h1->right, h2);
			
			// Maintain leftist property
			if (h1->left == nullptr) {
				h1->left = h1->right;
				h1->right = nullptr;
			} else {
				int leftNpl = (h1->left == nullptr) ? -1 : h1->left->npl;
				int rightNpl = (h1->right == nullptr) ? -1 : h1->right->npl;
				
				if (leftNpl < rightNpl) {
					Node *temp = h1->left;
					h1->left = h1->right;
					h1->right = temp;
				}
				
				h1->npl = ((h1->right == nullptr) ? -1 : h1->right->npl) + 1;
			}
			
			return h1;
		} catch (...) {
			throw;
		}
	}
	
public:
	/**
	 * @brief default constructor
	 */
	priority_queue() : root(nullptr), count(0) {}

	/**
	 * @brief copy constructor
	 * @param other the priority_queue to be copied
	 */
	priority_queue(const priority_queue &other) : root(nullptr), count(other.count), cmp(other.cmp) {
		if (other.root != nullptr) {
			root = copyTree(other.root);
		}
	}

	/**
	 * @brief deconstructor
	 */
	~priority_queue() {
		deleteTree(root);
	}

	/**
	 * @brief Assignment operator
	 * @param other the priority_queue to be assigned from
	 * @return a reference to this priority_queue after assignment
	 */
	priority_queue &operator=(const priority_queue &other) {
		if (this == &other) return *this;
		
		Node *newRoot = nullptr;
		if (other.root != nullptr) {
			newRoot = copyTree(other.root);
		}
		
		deleteTree(root);
		root = newRoot;
		count = other.count;
		cmp = other.cmp;
		
		return *this;
	}

	/**
	 * @brief get the top element of the priority queue.
	 * @return a reference of the top element.
	 * @throws container_is_empty if empty() returns true
	 */
	const T & top() const {
		if (root == nullptr) {
			throw container_is_empty();
		}
		return root->data;
	}

	/**
	 * @brief push new element to the priority queue.
	 * @param e the element to be pushed
	 */
	void push(const T &e) {
		Node *newNode = new Node(e);
		Node *oldRoot = root;
		size_t oldCount = count;
		
		try {
			root = safeMerge(root, newNode);
			count++;
		} catch (...) {
			// Restore state
			root = oldRoot;
			count = oldCount;
			delete newNode;
			throw;
		}
	}

	/**
	 * @brief delete the top element from the priority queue.
	 * @throws container_is_empty if empty() returns true
	 */
	void pop() {
		if (root == nullptr) {
			throw container_is_empty();
		}
		
		Node *oldRoot = root;
		root = merge(root->left, root->right);
		delete oldRoot;
		count--;
	}

	/**
	 * @brief return the number of elements in the priority queue.
	 * @return the number of elements.
	 */
	size_t size() const {
		return count;
	}

	/**
	 * @brief check if the container is empty.
	 * @return true if it is empty, false otherwise.
	 */
	bool empty() const {
		return count == 0;
	}

	/**
	 * @brief merge another priority_queue into this one.
	 * The other priority_queue will be cleared after merging.
	 * The complexity is at most O(logn).
	 * @param other the priority_queue to be merged.
	 */
	void merge(priority_queue &other) {
		if (this == &other) return;
		
		Node *oldRoot = root;
		size_t oldCount = count;
		Node *otherOldRoot = other.root;
		size_t otherOldCount = other.count;
		
		try {
			root = safeMerge(root, other.root);
			count += other.count;
			other.root = nullptr;
			other.count = 0;
		} catch (...) {
			// Restore both queues
			root = oldRoot;
			count = oldCount;
			other.root = otherOldRoot;
			other.count = otherOldCount;
			throw;
		}
	}
};

}

#endif
