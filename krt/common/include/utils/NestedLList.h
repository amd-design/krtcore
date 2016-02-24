/*****************************************************************************
*
*  PROJECT:     ATG
*  FILE:        NestedLList.hpp
*  PURPOSE:     Nested linked lists a'la RW
*
*****************************************************************************/

#ifndef _RENDERWARE_LIST_DEFINITIONS_
#define _RENDERWARE_LIST_DEFINITIONS_

// Macros used by RW, optimized for usage by the engine (inspired by S2 Games' macros)
#ifdef USE_MACRO_LIST

#define LIST_ISVALID(item) ( (item).next->prev == &(item) && (item).prev->next == &(item) )
#define LIST_VALIDATE(item) ( assert( LIST_ISVALID( (item) ) ) )
#define LIST_APPEND(link, item) ( (item).next = &(link), (item).prev = (link).prev, (item).prev->next = &(item), (item).next->prev = &(item) )
#define LIST_INSERT(link, item) ( (item).next = (link).next, (item).prev = &(link), (item).prev->next = &(item), (item).next->prev = &(item) )
#define LIST_REMOVE(link) ( (link).prev->next = (link).next, (link).next->prev = (link).prev )
#define LIST_CLEAR(link) ( (link).prev = &(link), (link).next = &(link) )
#define LIST_INITNODE(link) ( (link).prev = NULL, (link).next = NULL )
#ifdef _DEBUG
#define LIST_EMPTY(link) ( (link).prev == &(link) && (link).next == &(link) )
#else
#define LIST_EMPTY(link) ( (link).next == &(link) )
#endif //_DEBUG

#else //USE_MACRO_LIST

// Optimized versions.
// Not prone to bugs anymore.
template <typename listType>    inline bool LIST_ISVALID( listType& item )                      { return item.next->prev == &item && item.prev->next == &item; }
template <typename listType>    inline void LIST_VALIDATE( listType& item )                     { return assert( LIST_ISVALID( item ) ); }
template <typename listType>    inline void LIST_APPEND( listType& link, listType& item )       { item.next = &link; item.prev = link.prev; item.prev->next = &item; item.next->prev = &item; }
template <typename listType>    inline void LIST_INSERT( listType& link, listType& item )       { item.next = link.next; item.prev = &link; item.prev->next = &item; item.next->prev = &item; }
template <typename listType>    inline void LIST_REMOVE( listType& link )                       { link.prev->next = link.next; link.next->prev = link.prev; }
template <typename listType>    inline void LIST_CLEAR( listType& link )                        { link.prev = &link; link.next = &link; }
template <typename listType>    inline void LIST_INITNODE( listType& link )                     { link.prev = NULL; link.next = NULL; }
template <typename listType>    inline bool LIST_EMPTY( listType& link )                        { return link.prev == &link && link.next == &link; }

#endif //USE_MACRO_LIST

// These have to be macros unfortunately.
// Special macros used by RenderWare only.
#define LIST_GETITEM(type, item, node) ( (type*)( (char*)(item) - offsetof(type, node) ) )
#define LIST_FOREACH_BEGIN(type, root, node) for ( NestedListEntry <type> *iter = (root).next, *niter; iter != &(root); iter = niter ) { type *item = LIST_GETITEM(type, iter, node); niter = iter->next;
#define LIST_FOREACH_END }

template < class type, int _node_offset = -1 >
struct NestedListEntry
{
    NestedListEntry <type> *next, *prev;
};
template < class type, int _node_offset = -1 >
struct NestedList
{
    NestedListEntry <type> root;
};

#endif //_RENDERWARE_LIST_DEFINITIONS_