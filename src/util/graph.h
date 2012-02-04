/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VENOM_UTIL_GRAPH_H
#define VENOM_UTIL_GRAPH_H

#include <cassert>

#include <algorithm>
#include <functional>
#include <map>
#include <vector>
#include <set>
#include <stdexcept>
#include <utility>

namespace venom {
namespace util {

template <typename T,
          typename Compare = std::less<T> >
class directed_graph {
public:
  typedef T value_type;
  typedef size_t size_type;

  // regular construct
  directed_graph() {}

  // copy construct
  directed_graph(const directed_graph<T>& that) { operator=(that); }

  // assignment op
  directed_graph& operator=(const directed_graph<T>& that) {
    _nodes       = that._nodes;
    _node_lookup = that._node_lookup;
    reset_iterator();
    return *this;
  }

  /**
   * Add elem to this graph. Returns true if
   * elem did not exist (and thus was added), or false
   * if it already exists (and thus nothing changed)
   */
  bool add_elem(const T& elem) {
    reset_iterator();
    bool create;
    resolve_node(elem, create);
    return create;
  }

  /**
   * Add edge [from -> to] to this graph
   */
  bool add_edge(const T& from, const T& to) {
    reset_iterator();
    bool create;
    size_type from_idx = resolve_node(from, create);
    size_type to_idx   = resolve_node(to, create);

    node_type& from_node = _nodes[from_idx];
    node_type& to_node   = _nodes[to_idx];

    std::set<size_type>::iterator it =
      from_node._outgoing.find(to_idx);
    if (it == from_node._outgoing.end()) {
      // new edge
      from_node._outgoing.insert(to_idx);
      assert(to_node._incoming.find(from_idx) ==
             to_node._incoming.end());
      to_node._incoming.insert(from_idx);
      return true;
    } else {
      assert(to_node._incoming.find(from_idx) !=
             to_node._incoming.end());
      return false;
    }
  }

  size_type size() const { return _nodes.size(); }

private:

  // internal node data structure

  struct _node {
    _node(const T& _elem, size_t _id)
      : _elem(_elem), _id(_id) {}

    T _elem;
    size_type _id;

    // edges from this node (outgoing edges)
    std::set<size_type> _outgoing;

    // edges *to* this node (incoming edges)
    std::set<size_type> _incoming;

  };
  typedef _node node_type;

  typedef std::vector<node_type> node_vec;
  node_vec _nodes;

  typedef std::map<T, size_type, Compare> node_map;
  node_map _node_lookup;

  // internal iterator state data structure

  struct _iter_state {
    _iter_state()
      : _valid(false), _nodes_p(NULL) {}
    inline void reset()
      { _sorted.clear(); _valid = false; _nodes_p = NULL; }

    std::vector<size_type> _sorted;

    bool _valid;
    const std::vector<node_type>* _nodes_p;
  };
  typedef _iter_state iter_state_type;
  mutable iter_state_type _istate;

  // iterator data structure

  class _tsort_const_iterator {
    friend class directed_graph<T, Compare>;
  protected:
    _tsort_const_iterator(iter_state_type& _istate, bool end)
      : _istate_p(&_istate), _pos(end ? _istate._sorted.size() : 0) {}

  public:
    _tsort_const_iterator() : _istate_p(NULL), _pos(0) {}

    _tsort_const_iterator(const _tsort_const_iterator& that) {
      operator=(that);
    }
    _tsort_const_iterator& operator=(const _tsort_const_iterator& that) {
      _istate_p = that._istate_p;
      _pos      = that._pos;
      return *this;
    }

    inline _tsort_const_iterator& operator++() { _pos++; return *this; }
    inline _tsort_const_iterator operator++(int _unused) {
      _tsort_const_iterator that = *this;
      return that++;
    }

    inline _tsort_const_iterator& operator--() { _pos--; return *this; }
    inline _tsort_const_iterator operator--(int _unused) {
      _tsort_const_iterator that = *this;
      return that--;
    }

    inline bool operator==(const _tsort_const_iterator& that) {
      return _istate_p == that._istate_p &&
             _pos == that._pos;
    }

    inline bool operator!=(const _tsort_const_iterator& that) {
      return !operator==(that);
    }

    inline const T& operator*() {
      return (*_istate_p->_nodes_p)[_istate_p->_sorted[_pos]]._elem;
    }

    inline const T* operator->() { return &(operator*()); }

  private:
    const iter_state_type* _istate_p;
    size_type _pos;
  };

  // dfs support

  /** maps node_id -> (previsit, postvisit) */
  typedef std::map< size_type, std::pair< size_type, size_type > >
          dfs_track_type;

  void dfs(dfs_track_type& ctx) const {
    size_type counter = 0;
    for (typename node_vec::const_iterator it = _nodes.begin();
         it != _nodes.end(); ++it) {
      dfs_impl(*it, ctx, counter);
    }
  }

  void dfs_impl(
      const node_type& node, dfs_track_type& ctx, size_type& counter) const {

    if (ctx.find(node._id) != ctx.end()) return;
    ctx[node._id] = std::make_pair(counter++, 0); // previsit
    for (std::set<size_type>::const_iterator it = node._outgoing.begin();
         it != node._outgoing.end(); ++it) {
      dfs_impl(_nodes[*it], ctx, counter);
    }
    ctx[node._id].second = counter++; // postvisit
  }

public:
  typedef _tsort_const_iterator tsort_const_iterator;

  bool has_cycle() const {
    dfs_track_type ctx;
    dfs(ctx);
    // look for any edge (u, v) st post(u) < post(v).
    // if so, there exists a cycle
    for (typename node_vec::const_iterator it = _nodes.begin();
         it != _nodes.end(); ++it) {
      assert(ctx.find(it->_id) != ctx.end());
      std::pair<size_type, size_type>& u = ctx[it->_id];
      assert(u.first != u.second);
      for (std::set<size_type>::const_iterator iit = it->_outgoing.begin();
           iit != it->_outgoing.end(); ++iit) {
        assert(ctx.find(*iit) != ctx.end());
        std::pair<size_type, size_type>& v = ctx[*iit];
        assert(v.first != v.second);
        if (u.second < v.second) return true;
      }
    }
    return false;
  }

  // iterators undefined if has_cycle() is true

  inline tsort_const_iterator tsort_begin() const {
    init_iterator();
    return tsort_const_iterator(_istate, false);
  }

  tsort_const_iterator tsort_end() {
    init_iterator();
    return tsort_const_iterator(_istate, true);
  }

  // no regular (non-const) iterator, since the tsort is really
  // just a view

private:

  /** gets/creates a new node for elem */
  size_type resolve_node(const T& elem, bool& create) {
    size_type idx = _nodes.size();
    std::pair<typename node_map::iterator, bool> res =
      _node_lookup.insert(std::make_pair(elem, idx));
    if (res.second) {
      // new elem
      _nodes.push_back(node_type(elem, idx));
      create = true;
      return idx;
    }
    create = false;
    return res.first->second;
  }

  // iterator support

  inline void init_iterator() const {
    if (!_istate._valid) {
      std::set<size_type> seen_already;
      for (typename node_vec::const_iterator it = _nodes.begin();
           it != _nodes.end(); ++it) {
        if (it->_outgoing.empty()) {
          visit(*it, _istate._sorted, seen_already);
        }
      }
      _istate._valid   = true;
      _istate._nodes_p = &_nodes;
    }
  }

  inline void reset_iterator() { _istate.reset(); }

  void visit(const node_type& node,
             std::vector<size_type>& sorted,
             std::set<size_type>& seen_already) const {
    if (seen_already.find(node._id) != seen_already.end()) return;
    seen_already.insert(node._id);
    for (std::set<size_type>::const_iterator it = node._incoming.begin();
         it != node._incoming.end(); ++it) {
      visit(_nodes[*it], sorted, seen_already);
    }
    sorted.push_back(node._id);
  }

};

} // namespace util
} // namespace venom

#endif /* VENOM_UTIL_GRAPH_H */
