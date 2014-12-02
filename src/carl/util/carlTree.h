/**
 * @file Tree.h
 * @author Gereon Kremer <gereon.kremer@cs.rwth-aachen.de>
 */

#pragma once

#include <iterator>
#include <list>
#include <limits>
#include <stack>
#include <type_traits>
#include <vector>

#include "../io/streamingOperators.h"

#define USE_DENSE_MEMORY

namespace carl {

/**
 * This class represents a tree.
 *
 * It tries to stick to the STL style as close as possible.
 */
#ifdef USE_DENSE_MEMORY
template<typename T>
class Tree {
private:
	static constexpr std::size_t MAXINT = std::numeric_limits<std::size_t>::max();
	struct Node {
		std::size_t id;
		mutable T data;
		std::size_t parent;
		std::size_t previousSibling = MAXINT;
		std::size_t nextSibling = MAXINT;
		std::size_t firstChild = MAXINT;
		std::size_t lastChild = MAXINT;
		std::size_t depth;
		Node(std::size_t id, const T& data, std::size_t parent, std::size_t depth):
			id(id), data(data), parent(parent), depth(depth)
		{
		}
		bool operator==(const Node& n) {
			return this == &n;
		}
		void updateDepth(std::size_t newDepth) {
			depth = newDepth;
			//for (auto& c: children) nodes[c].updateDepth(newDepth + 1);
		}
	};
	std::vector<Node> nodes;
	std::size_t emptyNodes = MAXINT;
protected:
	/**
	 * This is the base class for all iterators.
	 * It takes care of correct implementation of all operators and reversion.
	 *
	 * An actual iterator `T<reverse>` only has to
	 * - inherit from `BaseIterator<T, reverse>`,
	 * - provide appropriate constructors,
	 * - implement `next()` and `previous()`.
	 * If the iterator supports only forward iteration, it omits the template
	 * argument, inherits from `BaseIterator<T, false>` and does not implement
	 * `previous()`.
	 */
	template<typename Iterator, bool reverse>
	struct BaseIterator {
		friend Tree;
	protected:
		const Tree<T>* tree;
		BaseIterator(const Tree<T>* t, std::size_t root): tree(t), current(root) {}
	public:
		std::size_t current;
		BaseIterator(const BaseIterator& ii): tree(ii.tree), current(ii.current) {}
		BaseIterator(BaseIterator&& ii): tree(ii.tree), current(ii.current) {}
		template<typename It, bool r>
		BaseIterator(const BaseIterator<It,r>& ii): tree(ii.tree), current(ii.current) {}
		BaseIterator& operator=(const BaseIterator& ii) {
			this->tree = ii.tree;
			this->current = ii.current;
			return *this;
		}
		BaseIterator& operator=(BaseIterator&& ii) {
			this->tree = ii.tree;
			this->current = ii.current;
			return *this;
		}
		std::size_t depth() const {
			assert(current != MAXINT);
			return tree->nodes[current].depth;
		}
		T& operator*() {
			return tree->nodes[current].data;
		}
		const T& operator*() const {
			return tree->nodes[current].data;
		}

		template<typename I = Iterator>
		typename std::enable_if<!reverse,I>::type& operator++() {
			return static_cast<Iterator*>(this)->next();
		}
		template<typename I = Iterator>
		typename std::enable_if<reverse,I>::type& operator++() {
			return static_cast<Iterator*>(this)->previous();
		}
		template<typename I = Iterator>
		typename std::enable_if<!reverse,I>::type operator++(int) {
			return static_cast<Iterator*>(this)->next();
		}
		template<typename I = Iterator>
		typename std::enable_if<reverse,I>::type operator++(int) {
			return static_cast<Iterator*>(this)->previous();
		}
		template<typename I = Iterator>
		typename std::enable_if<!reverse,I>::type& operator--() {
			return static_cast<Iterator*>(this)->previous();
		}
		template<typename I = Iterator>
		typename std::enable_if<reverse,I>::type& operator--() {
			return static_cast<Iterator*>(this)->next();
		}
		template<typename I = Iterator>
		typename std::enable_if<!reverse,I>::type operator--(int) {
			return static_cast<Iterator*>(this)->previous();
		}
		template<typename I = Iterator>
		typename std::enable_if<reverse,I>::type operator--(int) {
			return static_cast<Iterator*>(this)->next();
		}
	};
public:
	template<typename I, bool r>
	friend bool operator==(const BaseIterator<I,r>& i1, const BaseIterator<I,r>& i2) {
		return i1.current == i2.current;
	}
	template<typename I, bool r>
	friend bool operator!=(const BaseIterator<I,r>& i1, const BaseIterator<I,r>& i2) {
		return i1.current != i2.current;
	}

	/**
	 * Iterator class for pre-order iterations over all elements.
	 */
	template<bool reverse = false>
	struct PreorderIterator:
		public BaseIterator<PreorderIterator<reverse>, reverse>,
		public std::iterator<std::bidirectional_iterator_tag, T, std::size_t, T*, T&>
	{
		friend Tree;
	protected:
		typedef BaseIterator<PreorderIterator<reverse>, reverse> Base;
		PreorderIterator(const Tree<T>* t): Base(t, MAXINT) {}
		PreorderIterator(const Tree<T>* t, std::size_t root): Base(t, root) {}
		PreorderIterator& next() {
			if (this->current == MAXINT) {
				this->current = this->tree->begin_preorder().current;
			} else if (this->tree->nodes[this->current].firstChild == MAXINT) {
				while (this->tree->nodes[this->current].nextSibling == MAXINT) {
					this->current = this->tree->nodes[this->current].parent;
					if (this->current == MAXINT) return *this;
				}
				this->current = this->tree->nodes[this->current].nextSibling;
			} else {
				this->current = this->tree->nodes[this->current].firstChild;
			}
			return *this;
		}
		PreorderIterator& previous() {
			if (this->current == MAXINT) {
				this->current = this->tree->rbegin_preorder().current;
			} else if (this->tree->nodes[this->current].previousSibling == MAXINT) {
				this->current = this->tree->nodes[this->current].parent;
			} else {
				this->current = this->tree->nodes[this->current].previousSibling;
				while (this->tree->nodes[this->current].firstChild != MAXINT) {
					this->current = this->tree->nodes[this->current].lastChild;
				}
			}
			return *this;
		}
	public:
		template<typename It>
		PreorderIterator(const BaseIterator<It,reverse>& ii): Base(ii) {}
		PreorderIterator(const PreorderIterator& ii): Base(ii) {}
		PreorderIterator(PreorderIterator&& ii): Base(ii) {}
		PreorderIterator& operator=(const PreorderIterator& it) {
			Base::operator=(it);
			return *this;
		}
		PreorderIterator& operator=(PreorderIterator&& it) {
			Base::operator=(it);
			return *this;
		}
		virtual ~PreorderIterator() {}
	};
	static_assert(std::is_copy_constructible<PreorderIterator<false>>::value, "");
	static_assert(std::is_move_constructible<PreorderIterator<false>>::value, "");
	static_assert(std::is_destructible<PreorderIterator<false>>::value, "");
	static_assert(std::is_copy_constructible<PreorderIterator<true>>::value, "");
	static_assert(std::is_move_constructible<PreorderIterator<true>>::value, "");
	static_assert(std::is_destructible<PreorderIterator<true>>::value, "");

	/**
	 * Iterator class for post-order iterations over all elements.
	 */
	template<bool reverse = false>
	struct PostorderIterator:
		public BaseIterator<PostorderIterator<reverse>,reverse>,
		public std::iterator<std::bidirectional_iterator_tag, T, std::size_t, T*, T&>
	{
		friend Tree;
	protected:
		typedef BaseIterator<PostorderIterator<reverse>,reverse> Base;
		PostorderIterator(const Tree<T>* t): Base(t, MAXINT) {}
		PostorderIterator(const Tree<T>* t, std::size_t root): Base(t, root) {}
		PostorderIterator& next() {
			if (this->current == MAXINT) {
				this->current = this->tree->begin_postorder().current;
			} else  if (this->tree->nodes[this->current].nextSibling == MAXINT) {
				this->current = this->tree->nodes[this->current].parent;
			} else {
				this->current = this->tree->nodes[this->current].nextSibling;
				while (this->tree->nodes[this->current].firstChild != MAXINT) {
					this->current =	this->tree->nodes[this->current].firstChild;
				}
			}
			return *this;
		}
		PostorderIterator& previous() {
			if (this->current == MAXINT) {
				this->current = this->tree->rbegin_postorder().current;
			} else if (this->tree->nodes[this->current].firstChild == MAXINT) {
				if (this->tree->nodes[this->current].previousSibling != MAXINT) {
					this->current = this->tree->nodes[this->current].previousSibling;
				} else {
					while (this->tree->nodes[this->current].previousSibling == MAXINT) {
						this->current = this->tree->nodes[this->current].parent;
						if (this->current == MAXINT) return *this;
					}
					this->current = this->tree->nodes[this->current].previousSibling;
				}
			} else {
				this->current = this->tree->nodes[this->current].lastChild;
			}
			return *this;
		}
	public:
		template<typename It>
		PostorderIterator(const BaseIterator<It,reverse>& ii): Base(ii) {}
		PostorderIterator(const PostorderIterator& ii): Base(ii) {}
		PostorderIterator(PostorderIterator&& ii): Base(ii) {}
		PostorderIterator& operator=(const PostorderIterator& it) {
			Base::operator=(it);
			return *this;
		}
		PostorderIterator& operator=(PostorderIterator&& it) {
			Base::operator=(it);
			return *this;
		}
		virtual ~PostorderIterator() {}
	};
	static_assert(std::is_copy_constructible<PostorderIterator<false>>::value, "");
	static_assert(std::is_move_constructible<PostorderIterator<false>>::value, "");
	static_assert(std::is_destructible<PostorderIterator<false>>::value, "");
	static_assert(std::is_copy_constructible<PostorderIterator<true>>::value, "");
	static_assert(std::is_move_constructible<PostorderIterator<true>>::value, "");
	static_assert(std::is_destructible<PostorderIterator<true>>::value, "");

	/**
	 * Iterator class for iterations over all leaf elements.
	 */
	template<bool reverse = false>
	struct LeafIterator:
		public BaseIterator<LeafIterator<reverse>, reverse>,
		public std::iterator<std::bidirectional_iterator_tag, T, std::size_t, T*, T&>
	{
		friend Tree;
	protected:
		typedef BaseIterator<LeafIterator<reverse>,reverse> Base;
		LeafIterator(const Tree<T>* t): Base(t, MAXINT) {}
		LeafIterator(const Tree<T>* t, std::size_t root): Base(t, root) {}
		LeafIterator& next() {
			if (this->current == MAXINT) {
				this->current = this->tree->begin_leaf().current;
			} else {
				std::size_t target = this->tree->nodes[this->current].depth;
				PreorderIterator<false> it(this->tree, this->current);
				do {
					++it;
					if (it.current == MAXINT) break;
				} while (this->tree->nodes[it.current].depth != target);
				this->current = it.current;
			}
			return *this;
		}
		LeafIterator& previous() {
			if (this->current == MAXINT) {
				this->current = this->tree->rbegin_leaf().current;
			} else {
				std::size_t target = this->tree->nodes[this->current].depth;
				PreorderIterator<false> it(this->tree, this->current);
				do {
					it.previous();
					if (it.current == MAXINT) break;
				} while (this->tree->nodes[it.current].depth != target);
				this->current = it.current;
			}
			return *this;
		}
	public:
		template<typename It>
		LeafIterator(const BaseIterator<It,reverse>& ii): Base(ii) {}
		LeafIterator(const LeafIterator& ii): Base(ii) {}
		LeafIterator(LeafIterator&& ii): Base(this->tree, ii.current) {}
		LeafIterator& operator=(const LeafIterator& it) {
			Base::operator=(it);
			return *this;
		}
		LeafIterator& operator=(LeafIterator&& it) {
			Base::operator=(it);
			return *this;
		}
		virtual ~LeafIterator() {}
	};
	static_assert(std::is_copy_constructible<LeafIterator<false>>::value, "");
	static_assert(std::is_move_constructible<LeafIterator<false>>::value, "");
	static_assert(std::is_destructible<LeafIterator<false>>::value, "");
	static_assert(std::is_copy_constructible<LeafIterator<true>>::value, "");
	static_assert(std::is_move_constructible<LeafIterator<true>>::value, "");
	static_assert(std::is_destructible<LeafIterator<true>>::value, "");

	/**
	 * Iterator class for iterations over all elements of a certain depth.
	 */
	template<bool reverse = false>
	struct DepthIterator:
		public BaseIterator<DepthIterator<reverse>,reverse>,
		public std::iterator<std::bidirectional_iterator_tag, T, std::size_t, T*, T&>
	{
		friend Tree;
	protected:
		typedef BaseIterator<DepthIterator<reverse>,reverse> Base;
		std::size_t depth;
		DepthIterator(const Tree<T>* t): Base(t, MAXINT), depth(0) {}
		DepthIterator(const Tree<T>* t, std::size_t root, std::size_t depth): Base(t, root), depth(depth) {
			assert(!this->tree->nodes.empty());
			if (reverse) {
				PostorderIterator<reverse> it(this->tree, this->current);
				while (it.current != MAXINT && it.depth() != depth) ++it;
				this->current = it.current;
			} else {
				PreorderIterator<reverse> it(this->tree, this->current);
				while (it.current != MAXINT && it.depth() != depth) ++it;
				this->current = it.current;
			}
		}
		DepthIterator& next() {
			if (this->current == MAXINT) {
				this->current = this->tree->begin_depth(depth).current;
			} else if (this->tree->nodes[this->current].nextSibling == MAXINT) {
				std::size_t target = this->tree->nodes[this->current].depth;
				while (this->tree->nodes[this->current].nextSibling == MAXINT) {
					this->current = this->tree->nodes[this->current].parent;
					if (this->current == MAXINT) return *this;
				}
				PreorderIterator<reverse> it(this->tree, this->tree->nodes[this->current].nextSibling);
				for (; it.current != MAXINT; ++it) {
					if (it.depth() == target) break;
				}
				this->current = it.current;
			} else {
				this->current = this->tree->nodes[this->current].nextSibling;
			}
			return *this;
		}
		DepthIterator& previous() {
			if (this->current == MAXINT) {
				this->current = this->tree->rbegin_depth(depth).current;
			} else if (this->tree->nodes[this->current].previousSibling == MAXINT) {
				std::size_t target = this->tree->nodes[this->current].depth;
				while (this->tree->nodes[this->current].previousSibling == MAXINT) {
					this->current = this->tree->nodes[this->current].parent;
					if (this->current == MAXINT) return *this;
				}
				PostorderIterator<reverse> it(this->tree, this->tree->nodes[this->current].previousSibling);
				for (; it.current != MAXINT; ++it) {
					if (it.depth() == target) break;
				}
				this->current = it.current;
			} else {
				this->current = this->tree->nodes[this->current].previousSibling;
			}
			return *this;
		}
	public:
		template<typename It>
		DepthIterator(const BaseIterator<It,reverse>& ii): Base(ii), depth(nodes[ii.current].depth) {}
		DepthIterator(const DepthIterator& ii): Base(ii), depth(ii.depth) {}
		DepthIterator(DepthIterator&& ii): Base(ii), depth(ii.depth) {}
		DepthIterator& operator=(const DepthIterator& it) {
			Base::operator=(it);
			depth = it.depth;
			return *this;
		}
		DepthIterator& operator=(DepthIterator&& it) {
			Base::operator=(it);
			depth = it.depth;
			return *this;
		}
		virtual ~DepthIterator() {}
	};
	static_assert(std::is_copy_constructible<DepthIterator<false>>::value, "");
	static_assert(std::is_move_constructible<DepthIterator<false>>::value, "");
	static_assert(std::is_destructible<DepthIterator<false>>::value, "");
	static_assert(std::is_copy_constructible<DepthIterator<true>>::value, "");
	static_assert(std::is_move_constructible<DepthIterator<true>>::value, "");
	static_assert(std::is_destructible<DepthIterator<true>>::value, "");

	/**
	 * Iterator class for iterations over all children of a given element.
	 */
	template<bool reverse = false>
	struct ChildrenIterator:
		public BaseIterator<ChildrenIterator<reverse>,reverse>,
		public std::iterator<std::bidirectional_iterator_tag, T, std::size_t, T*, T&>
	{
		friend Tree;
	protected:
		typedef BaseIterator<ChildrenIterator<reverse>,reverse> Base;
		std::size_t parent;
		ChildrenIterator(const Tree<T>* t, std::size_t base): Base(t, base) {
			parent = base;
			assert(base != MAXINT);
			if (this->tree->nodes[this->current].firstChild == MAXINT) this->current = MAXINT;
			else {
				if (reverse) {
					this->current = this->tree->nodes[this->current].lastChild;
				} else {
					this->current = this->tree->nodes[this->current].firstChild;
				}
			}
		}
		ChildrenIterator& next() {
			if (this->current == MAXINT) {
				this->current = this->tree->begin_children(PreorderIterator<false>(this->tree, parent)).current;
			} else {
				this->current = this->tree->nodes[this->current].nextSibling;
			}
			return *this;
		}
		ChildrenIterator& previous() {
			if (this->current == MAXINT) {
				this->current = this->tree->rbegin_children(PreorderIterator<false>(this->tree, parent)).current;
			} else {
				this->current = this->tree->nodes[this->current].previousSibling;
			}
			return *this;
		}
	public:
		template<typename It>
		ChildrenIterator(const BaseIterator<It,reverse>& ii): Base(ii), parent(MAXINT) {
			if (this->tree->is_valid(ii)) parent = this->tree->nodes[ii.current].parent;
		}
		ChildrenIterator(const ChildrenIterator& ii): Base(ii), parent(ii.parent) {}
		ChildrenIterator(ChildrenIterator&& ii): Base(ii), parent(ii.parent) {}
		ChildrenIterator& operator=(const ChildrenIterator& it) {
			Base::operator=(it);
			parent = it.parent;
			return *this;
		}
		ChildrenIterator& operator=(ChildrenIterator&& it) {
			Base::operator=(it);
			parent = it.parent;
			return *this;
		}
		virtual ~ChildrenIterator() {}
	};
	static_assert(std::is_copy_constructible<ChildrenIterator<false>>::value, "");
	static_assert(std::is_move_constructible<ChildrenIterator<false>>::value, "");
	static_assert(std::is_destructible<ChildrenIterator<false>>::value, "");
	static_assert(std::is_copy_constructible<ChildrenIterator<true>>::value, "");
	static_assert(std::is_move_constructible<ChildrenIterator<true>>::value, "");
	static_assert(std::is_destructible<ChildrenIterator<true>>::value, "");

	/**
	 * Iterator class for iterations from a given element to the root.
	 */
	struct PathIterator:
		public BaseIterator<PathIterator,false>,
		public std::iterator<std::forward_iterator_tag, T, std::size_t, T*, T&>
	{
		friend Tree;
	protected:
		typedef BaseIterator<PathIterator,false> Base;
		PathIterator(const Tree<T>* t, std::size_t root): Base(t, root) {}
		PathIterator& next() {
			if (this->current != MAXINT) {
				this->current = this->tree->nodes[this->current].parent;
			}
			return *this;
		}
	public:
		template<typename It>
		PathIterator(const BaseIterator<It,false>& ii): Base(ii) {}
		PathIterator(const PathIterator& ii): Base(ii) {}
		PathIterator(PathIterator&& ii): Base(ii) {}
		PathIterator& operator=(const PathIterator& it) {
			Base::operator=(it);
			return *this;
		}
		PathIterator& operator=(PathIterator&& it) {
			Base::operator=(it);
			return *this;
		}
		virtual ~PathIterator() {}
	};
	static_assert(std::is_copy_constructible<PathIterator>::value, "");
	static_assert(std::is_move_constructible<PathIterator>::value, "");
	static_assert(std::is_destructible<PathIterator>::value, "");

	typedef PreorderIterator<false> iterator;

	Tree() {}
	Tree(const Tree& t): nodes(t.nodes), emptyNodes(t.emptyNodes) {}
	Tree(Tree&& t): nodes(std::move(t.nodes)), emptyNodes(t.emptyNodes) {}
	Tree& operator=(const Tree& t) {
		nodes = t.nodes;
		emptyNodes = t.emptyNodes;
		return *this;
	}
	Tree& operator=(Tree&& t) {
		nodes = std::move(t.nodes);
		emptyNodes = t.emptyNodes;
		return *this;
	}

	iterator begin() const {
		return begin_preorder();
	}
	iterator end() const {
		return end_preorder();
	}
	iterator rbegin() const {
		return rbegin_preorder();
	}
	iterator rend() const {
		return rend_preorder();
	}

	PreorderIterator<false> begin_preorder() const {
		return PreorderIterator<false>(this, 0);
	}
	PreorderIterator<false> end_preorder() const {
		return PreorderIterator<false>(this);
	}
	PreorderIterator<true> rbegin_preorder() const {
		std::size_t cur = 0;
		while (nodes[cur].lastChild != MAXINT) cur = nodes[cur].lastChild;
		return PreorderIterator<true>(this, cur);
	}
	PreorderIterator<true> rend_preorder() const {
		return PreorderIterator<true>(this);
	}
	PostorderIterator<false> begin_postorder() const {
		std::size_t cur = 0;
		while (nodes[cur].firstChild != MAXINT) cur = nodes[cur].firstChild;
		return PostorderIterator<false>(this, cur);
	}
	PostorderIterator<false> end_postorder() const {
		return PostorderIterator<false>(this);
	}
	PostorderIterator<true> rbegin_postorder() const {
		return PostorderIterator<true>(this, 0);
	}
	PostorderIterator<true> rend_postorder() const {
		return PostorderIterator<true>(this);
	}
	LeafIterator<false> begin_leaf() const {
		std::size_t cur = 0;
		while (nodes[cur].firstChild != MAXINT) cur = nodes[cur].firstChild;
		return LeafIterator<false>(this, cur);
	}
	LeafIterator<false> end_leaf() const {
		return LeafIterator<false>(this);
	}
	LeafIterator<true> rbegin_leaf() const {
		std::size_t cur = 0;
		while (nodes[cur].lastChild != MAXINT) cur = nodes[cur].lastChild;
		return LeafIterator<true>(this, cur);
	}
	LeafIterator<true> rend_leaf() const {
		return LeafIterator<true>(this);
	}
	DepthIterator<false> begin_depth(std::size_t depth) const {
		return DepthIterator<false>(this, 0, depth);
	}
	DepthIterator<false> end_depth() const {
		return DepthIterator<false>(this);
	}
	DepthIterator<true> rbegin_depth(std::size_t depth) const {
		return DepthIterator<true>(this, 0, depth);
	}
	DepthIterator<true> rend_depth() const {
		return DepthIterator<true>(this);
	}
	template<typename Iterator>
	ChildrenIterator<false> begin_children(const Iterator& it) const {
		return ChildrenIterator<false>(this, it.current);
	}
	template<typename Iterator>
	ChildrenIterator<false> end_children(const Iterator& it) const {
		return ChildrenIterator<false>(this, it.current);
	}
	template<typename Iterator>
	ChildrenIterator<true> rbegin_children(const Iterator& it) const {
		return ChildrenIterator<true>(this, it.current);
	}
	template<typename Iterator>
	ChildrenIterator<true> rend_children(const Iterator& it) const {
		return ChildrenIterator<true>(this, it.current);
	}
	template<typename Iterator>
	PathIterator begin_path(const Iterator& it) const {
		return PathIterator(this, it.current);
	}
	PathIterator end_path() const {
		return PathIterator(this, MAXINT);
	}

	/**
	 * Retrieves the maximum depth of all elements.
	 * @return Maximum depth.
	 */
	std::size_t max_depth() const {
		std::size_t max = 0;
		for (auto it = begin_leaf(); it != end_leaf(); ++it) {
			if (it.depth() > max) max = it.depth();
		}
		return max;
	}
	template<typename Iterator>
	std::size_t max_depth(const Iterator& it) const {
		std::size_t max = 0;
		for (auto i = begin_children(it); i != end_children(it); ++i) {
			std::size_t d = max_depth(i);
			if (d > max + 1) max = d + 1;
		}
		return max;
	}

	/**
	 * Check if the given element is a leaf.
	 * @param it Iterator.
	 * @return If `it` is a leaf.
	 */
	template<typename Iterator>
	bool is_leaf(const Iterator& it) const {
		return it.current->firstChild == MAXINT;
	}
	/**
	 * Check if the given element is a leftmost child.
	 * @param it Iterator.
	 * @return If `it` is a leftmost child.
	 */
	template<typename Iterator>
	bool is_leftmost(const Iterator& it) const {
		return nodes[it.current].previousSibling == MAXINT;
	}
	/**
	 * Check if the given element is a rightmost child.
	 * @param it Iterator.
	 * @return If `it` is a rightmost child.
	 */
	template<typename Iterator>
	bool is_rightmost(const Iterator& it) const {
		return nodes[it.current].nextSibling == MAXINT;
	}
	template<typename Iterator>
	bool is_valid(const Iterator& it) const {
		std::size_t cur = emptyNodes;
		while (cur != MAXINT) {
			if (cur == it.current) return false;
			cur = nodes[cur].nextSibling;
		}
		return (it.current >= 0) && (it.current < nodes.size());
	}
	/**
	 * Retrieves the parent of an element.
	 * @param it Iterator.
	 * @return Parent of `it`.
	 */
	template<typename Iterator>
	Iterator get_parent(const Iterator& it) const {
		return Iterator(this, nodes[it.current].parent);
	}

	/**
	 * Sets the value of the root element.
	 * @param data Data.
	 * @return Iterator to the root.
	 */
	PreorderIterator<> setRoot(const T& data) {
		if (nodes.empty()) nodes.emplace_back(0, data, MAXINT, 0);
		else nodes[0].data = data;
		return PreorderIterator<>(this, MAXINT);
	}
	/**
	 * Clears the tree.
	 */
	void clear() {
		nodes.clear();
		emptyNodes = MAXINT;
	}
	/**
	 * Add the given data as last child of the root element.
	 * @param data Data.
	 * @return Iterator to inserted element.
	 */
	PreorderIterator<> insert(const T& data) {
		if (nodes.empty()) setRoot(T());
		return insert(PreorderIterator<>(this, 0), data);
	}
	/**
	 * Add the given data as last child of the given element.
	 * @param position Element.
	 * @param data Data.
	 * @return Iterator to inserted element.
	 */
	template<typename Iterator>
	Iterator insert(Iterator position, const T& data) {
		std::size_t id = newNode(data, position.current, nodes[position.current].depth + 1);
		return Iterator(this, id);
	}
	/**
	 * Append another tree as last child of the root element.
	 * @param tree Tree.
	 * @return Iterator to root of inserted subtree.
	 */
	PreorderIterator<> append(Tree&& tree) {
		if (nodes.empty()) std::swap(nodes, tree.nodes);
		return append(PreorderIterator<>(0), std::move(tree));
	}
	/**
	 * Append another tree as last child of the given element.
	 * @param position Element.
	 * @param tree Tree.
	 * @return Iterator to root of inserted subtree.
	 */
	template<typename Iterator>
	Iterator append(Iterator position, Tree&& data) {
		Node* r = data.root;
		r->updateDepth(position.depth() + 1);
		data.root = nullptr;
		r->parent = position.current;
		std::size_t id = position.current->children.size();
		if (id > 0) r->previousSibling = &(position.current->children.back());
		position.current->children.push_back(*r);
		if (id > 0) r->previousSibling->nextSibling = &(position.current->children.back());
		return Iterator(&(position.current->children.back()));
	}

	template<typename Iterator>
	const Iterator& replace(const Iterator& position, const T& data) {
		nodes[position.current].data = data;
		return position;
	}

	/**
	 * Erase the element at the given position.
	 * Returns an iterator to the next position.
	 * @param position Element.
	 * @return Next element.
	 */
	template<typename Iterator>
	Iterator erase(Iterator position) {
		std::size_t id = position.current;
		if (id == 0) {
			clear();
			++position;
			return position;
		}
		eraseChildren(id);
		++position;
		if (nodes[id].nextSibling != MAXINT) {
			nodes[nodes[id].nextSibling].previousSibling = nodes[id].previousSibling;
		}
		if (nodes[id].previousSibling != MAXINT) {
			nodes[nodes[id].previousSibling].nextSibling = nodes[id].nextSibling;
		}
		eraseNode(id);
		return position;
	}
	/**
	 * Erase all children of the given element.
	 * @param position Element.
	 */
	template<typename Iterator>
	void eraseChildren(const Iterator& position) {
		eraseChildren(position.current);
	}
	template<typename TT>
	friend std::ostream& operator<<(std::ostream& os, Tree<TT>& tree) {
		for (auto it = tree.begin_preorder(); it != tree.end_preorder(); it++) {
			os << std::string(it.depth(), '\t') << *it << std::endl;
		}
		return os;
	}

private:
	std::size_t newNode(const T& data, std::size_t parent, std::size_t depth) {
		std::size_t res = 0;
		if (emptyNodes == MAXINT) {
			nodes.emplace_back(nodes.size(), data, parent, depth);
			res = nodes.size() - 1;
		} else {
			std::size_t res = emptyNodes;
			emptyNodes = nodes[emptyNodes].nextSibling;
			nodes[res].data = data;
			nodes[res].parent = parent;
			nodes[res].depth = depth;
			nodes[res].nextSibling = MAXINT;
		}
		if (parent != MAXINT) {
			if (nodes[parent].lastChild != MAXINT) {
				nodes[nodes[parent].lastChild].nextSibling = res;
				nodes[res].previousSibling = nodes[parent].lastChild;
				nodes[parent].lastChild = res;
			} else {
				nodes[parent].firstChild = res;
				nodes[parent].lastChild = res;
			}
		}
		return res;
	}
	void eraseChildren(std::size_t id) {
		if (nodes[id].firstChild == MAXINT) return;
		std::size_t cur = nodes[id].firstChild;
		while (cur != MAXINT) {
			std::size_t tmp = cur;
			cur = nodes[cur].nextSibling;
			eraseNode(tmp);
		}
		nodes[id].firstChild = MAXINT;
		nodes[id].lastChild = MAXINT;
	}
	void eraseNode(std::size_t id) {
		assert(nodes[id].firstChild == MAXINT);
		nodes[id].nextSibling = emptyNodes;
		nodes[id].previousSibling = MAXINT;
	}
};

template<typename T>
constexpr std::size_t Tree<T>::MAXINT;

#else
template<typename T>
class Tree {
private:
	struct Node {
		T data;
		Node* parent;
		Node* previousSibling;
		Node* nextSibling;
		std::list<Node> children;
		std::size_t depth;
		Node(const T& data, Node* const parent): data(data), parent(parent), previousSibling(nullptr), nextSibling(nullptr) {
			if (parent == nullptr) depth = 0;
			else depth = parent->depth + 1;
		}
		bool operator==(const Node& n) {
			return this == &n;
		}
		void updateDepth(std::size_t newDepth) {
			depth = newDepth;
			for (auto& c: children) c.updateDepth(newDepth + 1);
		}
	};
	
	Node* root;
	
protected:
	/**
	 * This is the base class for all iterators.
	 * It takes care of correct implementation of all operators and reversion.
	 *
	 * An actual iterator `T<reverse>` only has to
	 * - inherit from `BaseIterator<T, reverse>`,
	 * - provide appropriate constructors,
	 * - implement `next()` and `previous()`.
	 * If the iterator supports only forward iteration, it omits the template
	 * argument, inherits from `BaseIterator<T, false>` and does not implement
	 * `previous()`.
	 */
	template<typename Iterator, bool reverse>
	struct BaseIterator {
		friend Tree;
	protected:
		Node* current;
		BaseIterator(Node* root): current(root) {}
	public:
		BaseIterator(const BaseIterator& ii): current(ii.current) {}
		BaseIterator(BaseIterator&& ii): current(ii.current) {}
		BaseIterator& operator=(const BaseIterator& ii) {
			this->current = ii.current;
			return *this;
		}
		BaseIterator& operator=(BaseIterator&& ii) {
			this->current = ii.current;
			return *this;
		}
		std::size_t depth() const {
			assert(current != nullptr);
			return current->depth;
		}
		T& operator*() {
			return this->current->data;
		}
		const T& operator*() const {
			return this->current->data;
		}

		template<typename I = Iterator>
		typename std::enable_if<!reverse,I>::type& operator++() {
			return static_cast<Iterator*>(this)->next();
		}
		template<typename I = Iterator>
		typename std::enable_if<reverse,I>::type& operator++() {
			return static_cast<Iterator*>(this)->previous();
		}
		template<typename I = Iterator>
		typename std::enable_if<!reverse,I>::type operator++(int) {
			return static_cast<Iterator*>(this)->next();
		}
		template<typename I = Iterator>
		typename std::enable_if<reverse,I>::type operator++(int) {
			return static_cast<Iterator*>(this)->previous();
		}
		template<typename I = Iterator>
		typename std::enable_if<!reverse,I>::type& operator--() {
			return static_cast<Iterator*>(this)->previous();
		}
		template<typename I = Iterator>
		typename std::enable_if<reverse,I>::type& operator-() {
			return static_cast<Iterator*>(this)->next();
		}
		template<typename I = Iterator>
		typename std::enable_if<!reverse,I>::type operator--(int) {
			return static_cast<Iterator*>(this)->previous();
		}
		template<typename I = Iterator>
		typename std::enable_if<reverse,I>::type operator--(int) {
			return static_cast<Iterator*>(this)->next();
		}
	};
public:
	
	template<typename Iterator>
	friend bool operator==(const Iterator& i1, const Iterator& i2) {
		return i1.current == i2.current;
	}
	template<typename Iterator>
	friend bool operator!=(const Iterator& i1, const Iterator& i2) {
		return i1.current != i2.current;
	}

	/**
	 * Iterator class for pre-order iterations over all elements.
	 */
	template<bool reverse = false>
	struct PreorderIterator: public BaseIterator<PreorderIterator<reverse>, reverse> {
		friend Tree;
	protected:
		typedef BaseIterator<PreorderIterator<reverse>, reverse> Base;
		PreorderIterator(): Base(nullptr) {}
		PreorderIterator(Node* root): Base(root) {}
		PreorderIterator& next() {
			if (this->current->children.empty()) {
				while (this->current->nextSibling == nullptr) {
					this->current = this->current->parent;
					if (this->current == nullptr) return *this;
				}
				this->current = this->current->nextSibling;
			} else {
				this->current = &this->current->children.front();
			}
			return *this;
		}
		PreorderIterator& previous() {
			if (this->current->previousSibling == nullptr) {
				this->current = this->current->parent;
			} else {
				this->current = this->current->previousSibling;
				while (!this->current->children.empty()) {
					this->current = &this->current->children.back();
				}
			}
			return *this;
		}
	public:
		PreorderIterator(const PreorderIterator& ii): Base(ii.current) {}
		PreorderIterator(PreorderIterator&& ii): Base(ii.current) {}
		virtual ~PreorderIterator() {}
	};
	static_assert(std::is_copy_constructible<PreorderIterator<false>>::value, "");
	static_assert(std::is_move_constructible<PreorderIterator<false>>::value, "");
	static_assert(std::is_destructible<PreorderIterator<false>>::value, "");
	static_assert(std::is_copy_constructible<PreorderIterator<true>>::value, "");
	static_assert(std::is_move_constructible<PreorderIterator<true>>::value, "");
	static_assert(std::is_destructible<PreorderIterator<true>>::value, "");

	/**
	 * Iterator class for post-order iterations over all elements.
	 */
	template<bool reverse = false>
	struct PostorderIterator: public BaseIterator<PostorderIterator<reverse>,reverse> {
		friend Tree;
	protected:
		typedef BaseIterator<PostorderIterator<reverse>,reverse> Base;
		PostorderIterator(): Base(nullptr) {}
		PostorderIterator(Node* root): Base(root) {}
		PostorderIterator& next() {
			if (this->current->nextSibling == nullptr) {
				this->current = this->current->parent;
			} else {
				this->current = this->current->nextSibling;
				while (!this->current->children.empty()) {
					this->current =	&this->current->children.front();
				}
			}
			return *this;
		}
		PostorderIterator& previous() {
			if (this->current->children.empty()) {
				if (this->current->previousSibling != nullptr) {
					this->current = this->current->previousSibling;
				} else {
					while (this->current->previousSibling == nullptr) {
						this->current = this->current->parent;
						if (this->current == nullptr) return *this;
					}
					this->current = this->current->previousSibling;
				}
			} else {
				this->current = &this->current->children.back();
			}
			return *this;
		}
	public:
		PostorderIterator(const PostorderIterator& ii): Base(ii.current) {}
		PostorderIterator(PostorderIterator&& ii): Base(ii.current) {}
		virtual ~PostorderIterator() {}
	};
	static_assert(std::is_copy_constructible<PostorderIterator<false>>::value, "");
	static_assert(std::is_move_constructible<PostorderIterator<false>>::value, "");
	static_assert(std::is_destructible<PostorderIterator<false>>::value, "");
	static_assert(std::is_copy_constructible<PostorderIterator<true>>::value, "");
	static_assert(std::is_move_constructible<PostorderIterator<true>>::value, "");
	static_assert(std::is_destructible<PostorderIterator<true>>::value, "");

	/**
	 * Iterator class for iterations over all leaf elements.
	 */
	template<bool reverse = false>
	struct LeafIterator: public BaseIterator<LeafIterator<reverse>,reverse> {
		friend Tree;
	protected:
		typedef BaseIterator<LeafIterator<reverse>,reverse> Base;
		PreorderIterator<false> it;
		LeafIterator(): Base(nullptr), it() {}
		LeafIterator(Node* root): Base(root), it(root) {}
		LeafIterator& next() {
			do {
				++it;
				if (it.current == nullptr) break;
			} while (!it.current->children.empty());
			this->current = it.current;
			return *this;
		}
		LeafIterator& previous() {
			do {
				--it;
				if (it.current == nullptr) break;
			} while (!it.current->children.empty());
			this->current = it.current;
			return *this;
		}
	public:
		LeafIterator(const LeafIterator& ii): Base(ii.current) {}
		LeafIterator(LeafIterator&& ii): Base(ii.current) {}
		virtual ~LeafIterator() {}
	};
	static_assert(std::is_copy_constructible<LeafIterator<false>>::value, "");
	static_assert(std::is_move_constructible<LeafIterator<false>>::value, "");
	static_assert(std::is_destructible<LeafIterator<false>>::value, "");
	static_assert(std::is_copy_constructible<LeafIterator<true>>::value, "");
	static_assert(std::is_move_constructible<LeafIterator<true>>::value, "");
	static_assert(std::is_destructible<LeafIterator<true>>::value, "");

	/**
	 * Iterator class for iterations over all elements of a certain depth.
	 */
	template<bool reverse = false>
	struct DepthIterator: public BaseIterator<DepthIterator<reverse>,reverse> {
		friend Tree;
	protected:
		typedef BaseIterator<DepthIterator<reverse>,reverse> Base;
		DepthIterator(): Base(nullptr) {}
		DepthIterator(Node* root, std::size_t depth): Base(root) {
			assert(root != nullptr);
			if (reverse) {
				PostorderIterator<reverse> it(this->current);
				while (it.depth() != depth) ++it;
				this->current = it.current;
			} else {
				PreorderIterator<reverse> it(this->current);
				while (it.depth() != depth) ++it;
				this->current = it.current;
			}
		}
		DepthIterator& next() {
			if (this->current->nextSibling == nullptr) {
				std::size_t target = this->current->depth;
				while (this->current->nextSibling == nullptr) {
					this->current = this->current->parent;
					if (this->current == nullptr) return *this;
				}
				PreorderIterator<reverse> it(this->current->nextSibling);
				for (; it.current != nullptr; ++it) {
					if (it.depth() == target) break;
				}
				this->current = it.current;
			} else {
				this->current = this->current->nextSibling;
			}
			return *this;
		}
		DepthIterator& previous() {
			if (this->current->previousSibling == nullptr) {
				std::size_t target = this->current->depth;
				while (this->current->previousSibling == nullptr) {
					this->current = this->current->parent;
					if (this->current == nullptr) return *this;
				}
				PostorderIterator<reverse> it(this->current->previousSibling);
				for (; it.current != nullptr; ++it) {
					if (it.depth() == target) break;
				}
				this->current = it.current;
			} else {
				this->current = this->current->previousSibling;
			}
			return *this;
		}
	public:
		DepthIterator(const DepthIterator& ii): Base(ii.current) {}
		DepthIterator(DepthIterator&& ii): Base(ii.current) {}
		virtual ~DepthIterator() {}
	};
	static_assert(std::is_copy_constructible<DepthIterator<false>>::value, "");
	static_assert(std::is_move_constructible<DepthIterator<false>>::value, "");
	static_assert(std::is_destructible<DepthIterator<false>>::value, "");
	static_assert(std::is_copy_constructible<DepthIterator<true>>::value, "");
	static_assert(std::is_move_constructible<DepthIterator<true>>::value, "");
	static_assert(std::is_destructible<DepthIterator<true>>::value, "");

	/**
	 * Iterator class for iterations over all children of a given element.
	 */
	template<bool reverse = false>
	struct ChildrenIterator: public BaseIterator<ChildrenIterator<reverse>,reverse> {
		friend Tree;
	protected:
		typedef BaseIterator<ChildrenIterator<reverse>,reverse> Base;
		ChildrenIterator(): Base(nullptr) {}
		ChildrenIterator(Node* base): Base(base) {
			assert(base != nullptr);
			if (this->current->children.empty()) this->current = nullptr;
			else {
				if (reverse) {
					this->current = &this->current->children.back();
				} else {
					this->current = &this->current->children.front();
				}
			}
		}
		ChildrenIterator& next() {
			this->current = this->current->nextSibling;
			return *this;
		}
		ChildrenIterator& previous() {
			this->current = this->current->previousSibling;
			return *this;
		}
	public:
		ChildrenIterator(const ChildrenIterator& ii): Base(ii.current) {}
		ChildrenIterator(ChildrenIterator&& ii): Base(ii.current) {}
		virtual ~ChildrenIterator() {}
	};
	static_assert(std::is_copy_constructible<ChildrenIterator<false>>::value, "");
	static_assert(std::is_move_constructible<ChildrenIterator<false>>::value, "");
	static_assert(std::is_destructible<ChildrenIterator<false>>::value, "");
	static_assert(std::is_copy_constructible<ChildrenIterator<true>>::value, "");
	static_assert(std::is_move_constructible<ChildrenIterator<true>>::value, "");
	static_assert(std::is_destructible<ChildrenIterator<true>>::value, "");

	/**
	 * Iterator class for iterations from a given element to the root.
	 */
	struct PathIterator: public BaseIterator<PathIterator,false> {
		friend Tree;
	protected:
		typedef BaseIterator<PathIterator,false> Base;
		PathIterator(Node* root): Base(root) {}
		PathIterator& next() {
			if (this->current != nullptr) {
				this->current = this->current->parent;
			}
			return *this;
		}
	public:
		PathIterator(const PathIterator& ii): Base(ii.current) {}
		PathIterator(PathIterator&& ii): Base(ii.current) {}
		virtual ~PathIterator() {}
	};
	static_assert(std::is_copy_constructible<PathIterator>::value, "");
	static_assert(std::is_move_constructible<PathIterator>::value, "");
	static_assert(std::is_destructible<PathIterator>::value, "");

	Tree() {
		root = nullptr;
	}

	PreorderIterator<false> begin_preorder() const {
		return PreorderIterator<false>(root);
	}
	PreorderIterator<false> end_preorder() const {
		return PreorderIterator<false>();
	}
	PreorderIterator<true> rbegin_preorder() const {
		Node* cur = root;
		while (!cur->children.empty()) cur = &(cur->children.back());
		return PreorderIterator<true>(cur);
	}
	PreorderIterator<true> rend_preorder() const {
		return PreorderIterator<true>();
	}
	PostorderIterator<false> begin_postorder() const {
		Node* cur = root;
		while (!cur->children.empty()) cur = &(cur->children.front());
		return PostorderIterator<false>(cur);
	}
	PostorderIterator<false> end_postorder() const {
		return PostorderIterator<false>();
	}
	PostorderIterator<true> rbegin_postorder() const {
		return PostorderIterator<true>(root);
	}
	PostorderIterator<true> rend_postorder() const {
		return PostorderIterator<true>();
	}
	LeafIterator<false> begin_leaf() const {
		Node* cur = root;
		while (!cur->children.empty()) cur = &(cur->children.front());
		return LeafIterator<false>(cur);
	}
	LeafIterator<false> end_leaf() const {
		return LeafIterator<false>();
	}
	LeafIterator<true> rbegin_leaf() const {
		Node* cur = root;
		while (!cur->children.empty()) cur = &(cur->children.back());
		return LeafIterator<true>(cur);
	}
	LeafIterator<true> rend_leaf() const {
		return LeafIterator<true>();
	}
	DepthIterator<false> begin_depth(std::size_t depth) const {
		return DepthIterator<false>(root, depth);
	}
	DepthIterator<false> end_depth() const {
		return DepthIterator<false>();
	}
	DepthIterator<true> rbegin_depth(std::size_t depth) const {
		return DepthIterator<true>(root, depth);
	}
	DepthIterator<true> rend_depth() const {
		return DepthIterator<true>();
	}
	template<typename Iterator>
	ChildrenIterator<false> begin_children(const Iterator& it) const {
		return ChildrenIterator<false>(it.current);
	}
	ChildrenIterator<false> end_children() const {
		return ChildrenIterator<false>();
	}
	template<typename Iterator>
	ChildrenIterator<true> rbegin_children(const Iterator& it) const {
		return ChildrenIterator<true>(it.current);
	}
	ChildrenIterator<true> rend_children() const {
		return ChildrenIterator<true>();
	}
	template<typename Iterator>
	PathIterator begin_path(const Iterator& it) const {
		return PathIterator(it.current);
	}
	PathIterator end_path() const {
		return PathIterator(nullptr);
	}

	/**
	 * Retrieves the maximum depth of all elements.
     * @return Maximum depth.
     */
	std::size_t max_depth() const {
		std::size_t max = 0;
		for (auto it = begin_leaf(); it != end_leaf(); ++it) {
			if (it.depth() > max) max = it.depth();
		}
		return max;
	}

	/**
	 * Check if the given element is a leaf.
	 * @param it Iterator.
	 * @return If `it` is a leaf.
	 */
	template<typename Iterator>
	bool is_leaf(const Iterator& it) const {
		return it.current->children.empty();
	}
	/**
	 * Check if the given element is a leftmost child.
     * @param it Iterator.
     * @return If `it` is a leftmost child.
     */
	template<typename Iterator>
	bool is_leftmost(const Iterator& it) const {
		if (it.current->parent == nullptr) return true;
		return &it.current->parent->children.front() == it.current;
	}
	/**
	 * Check if the given element is a rightmost child.
     * @param it Iterator.
     * @return If `it` is a rightmost child.
     */
	template<typename Iterator>
	bool is_rightmost(const Iterator& it) const {
		if (it.current->parent == nullptr) return true;
		return &it.current->parent->children.back() == it.current;
	}
	/**
	 * Retrieves the parent of an element.
     * @param it Iterator.
     * @return Parent of `it`.
     */
	template<typename Iterator>
	Iterator get_parent(const Iterator& it) const {
		return Iterator(it.current->parent);
	}

	/**
	 * Sets the value of the root element.
     * @param data Data.
     * @return Iterator to the root.
     */
	PreorderIterator<> setRoot(const T& data) {
		if (root == nullptr) root = new Node(data, nullptr);
		else root->data = data;
		return PreorderIterator<>(root);
	}
	/**
	 * Clears the tree.
     */
	void clear() {
		delete root;
		root = nullptr;
	}
	/**
	 * Add the given data as last child of the root element.
     * @param data Data.
     * @return Iterator to inserted element.
     */
	PreorderIterator<> insert(const T& data) {
		if (root == nullptr) setRoot(T());
		return insert(PreorderIterator<>(root), data);
	}
	/**
	 * Add the given data as last child of the given element.
     * @param position Element.
     * @param data Data.
     * @return Iterator to inserted element.
     */
	template<typename Iterator>
	Iterator insert(Iterator position, const T& data) {
		Node n(data, position.current);
		std::size_t id = position.current->children.size();
		if (id > 0) n.previousSibling = &(position.current->children.back());
		position.current->children.push_back(n);
		if (id > 0) n.previousSibling->nextSibling = &(position.current->children.back());
		return Iterator(&(position.current->children.back()));
	}
	/**
	 * Append another tree as last child of the root element.
     * @param tree Tree.
     * @return Iterator to root of inserted subtree.
     */
	PreorderIterator<> append(Tree&& Tree) {
		if (root == nullptr) std::swap(root, Tree.root);
		return append(PreorderIterator<>(root), std::move(Tree));
	}
	/**
	 * Append another tree as last child of the given element.
	 * @param position Element.
     * @param tree Tree.
     * @return Iterator to root of inserted subtree.
     */
	template<typename Iterator>
	Iterator append(Iterator position, Tree&& data) {
		Node* r = data.root;
		r->updateDepth(position.depth() + 1);
		data.root = nullptr;
		r->parent = position.current;
		std::size_t id = position.current->children.size();
		if (id > 0) r->previousSibling = &(position.current->children.back());
		position.current->children.push_back(*r);
		if (id > 0) r->previousSibling->nextSibling = &(position.current->children.back());
		return Iterator(&(position.current->children.back()));
	}

	/**
	 * Erase the element at the given position.
	 * Returns an iterator to the next position.
     * @param position Element.
     * @return Next element.
     */
	template<typename Iterator>
	Iterator erase(Iterator position) {
		eraseChildren(position);
		Node* n = position.current;
		++position;
		if (n->parent == nullptr) clear();
		else {
			if (n->nextSibling != nullptr) {
				n->nextSibling->previousSibling = n->previousSibling;
			}
			if (n->previousSibling != nullptr) {
				n->previousSibling->nextSibling = n->nextSibling;
			}
			n->parent->children.remove(*n);
		}
		return position;
	}
	/**
	 * Erase all children of the given element.
     * @param position Element.
     */
	template<typename Iterator>
	void eraseChildren(const Iterator& position) {
		position.current->children.clear();
	}

	template<typename TT>
	friend std::ostream& operator<<(std::ostream& os, Tree<TT>& Tree) {
		for (auto it = Tree.begin_preorder(); it != Tree.end_preorder(); it++) {
			os << std::string(it.depth(), '\t') << *it << std::endl;
		}
		return os;
	}
};
#endif

}