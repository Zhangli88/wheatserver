// Implementation of double linked-list
//
// Copyright (c) 2013 The Wheatserver Author. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
/*
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
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


#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "memalloc.h"

struct list *createList()
{
    struct list *l = wmalloc(sizeof(struct list));
    l->first = l->last = NULL;
    l->len = 0;
    l->dup = NULL;
    l->match = NULL;
    l->free = NULL;
    return l;
}

void listClear(struct list *l)
{
    struct listNode *current, *next;
    unsigned long len = l->len;

    current = l->first;
    while (len--) {
        next = current->next;
        if (l->free)
            l->free((void*)current->value);
        wfree(current);
        current = next;
    }
    l->len = 0;
    l->first = l->last = NULL;
}

void freeList(struct list *l)
{
    listClear(l);
    wfree(l);
}

struct listNode *appendToListTail(struct list *l, void *ptr)
{
    struct listNode *new_node = wmalloc(sizeof(struct listNode));
    if (new_node == NULL)
        return NULL;
    if (l->dup)
        new_node->value = (void *)l->dup(ptr);
    else
        new_node->value = (void *)ptr;
    new_node->prev = l->last;
    new_node->next = NULL;

    if (l->len == 0)
        l->first = new_node;
    else
        l->last->next = new_node;
    l->last = new_node;
    l->len++;
    return new_node;
}

struct listNode *insertToListHead(struct list *l, void *ptr)
{
    struct listNode *new_node = wmalloc(sizeof(struct listNode));
    if (new_node == NULL)
        return NULL;
    if (l->dup)
        new_node->value = (void *)l->dup(ptr);
    else
        new_node->value = (void *)ptr;
    new_node->prev = NULL;
    new_node->next = l->first;

    if (l->len == 0)
        l->last = new_node;
    else
        l->first->prev = new_node;
    l->first = new_node;
    l->len++;
    return new_node;
}

void removeListNode(struct list *l, struct listNode *node)
{
    if (node == NULL)
        return ;

    if (node->prev)
        node->prev->next = node->next;
    else
        l->first = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else
        l->last = node->prev;
    if (l->free) l->free((void*)(node->value));
    l->len--;
    wfree(node);
}

void listRotate(struct list *l)
{
    if (listLength(l) < 2)
        return ;
    struct listNode *second = l->first->next;
    second->prev = NULL;
    l->first->prev = l->last;
    l->first->next = NULL;
    l->last->next = l->first;
    l->last = l->first;
    l->first = second;
}

struct listNode *searchListKey(struct list *l, void *key)
{
    struct listIterator *iter = listGetIterator(l, START_HEAD);
    struct listNode *current;

    while ((current = listNext(iter)) != NULL) {
        if (l->match) {
            if (l->match((void*)current->value, key)) {
                freeListIterator(iter);
                return current;
            }
        } else {
            if ((void *)current->value == key) {
                freeListIterator(iter);
                return current;
            }
        }
    }
    freeListIterator(iter);
    return NULL;
}

void listEach(struct list *l, void(*func)(void *))
{
    struct listNode *curr;

    curr = l->first;
    while (curr != NULL) {
        func(listNodeValue(curr));
        curr = curr->next;
    }
}

void listEach2(struct list *l, void(*func)(void *, void*), void *data)
{
    struct listNode *curr;

    curr = l->first;
    while (curr != NULL) {
        func(listNodeValue(curr), data);
        curr = curr->next;
    }

}

struct listIterator *listGetIterator(struct list *list, int direction)
{
    struct listIterator *iter = wmalloc(sizeof(struct listIterator));
    if (iter == NULL)
        return NULL;
    if (direction == START_HEAD) {
        iter->next = list->first;
        iter->direction = START_HEAD;
    }
    else {
        iter->next = list->last;
        iter->direction = START_TAIL;
    }
    return iter;
}

void freeListIterator(struct listIterator *iter)
{
    wfree(iter);
}

struct listNode *listNext(struct listIterator *iter)
{
    struct listNode *current = iter->next;
    if (current == NULL)
        return NULL;
    if (iter->direction == START_HEAD)
        iter->next = current->next;
    else if (iter->direction == START_TAIL)
        iter->next = current->prev;
    return current;
}

void listPrint(struct list *list)
{
    if (list == NULL)
        return ;
    struct listIterator *iter = listGetIterator(list, START_HEAD);
    struct listNode *current;
    int i = 0;
    while ((current = listNext(iter)) != NULL) {
        printf("%d: %s -> ", i, (char *)current->value);
        i++;
    }
    freeListIterator(iter);
    printf("\n");
}

#ifdef LIST_TEST_MAIN
#include "test_help.h"

void *dupInt(void *ptr)
{
    int *new_ptr = wmalloc(sizeof(int));
    *new_ptr = *(int *)ptr;
    return new_ptr;
}

void freeInt(void *ptr)
{
    wfree(ptr);
}

int matchInt(void *ptr, void *key)
{
    return (*(int *)ptr == *(int *)key);
}

int main(void)
{
    {
        struct list *l;
        l = createList();
        test_cond("Create a list and obtain the length",
                listLength(l) == 0 && l != NULL);
        freeList(l);

        l = createList();
        int i;
        for (i = 0; i < 10; i++) {
            int *n = wmalloc(sizeof(int));
            *n = i;
            appendToListTail(l, n);
        }
        test_cond("Append to list tail length", listLength(l) == 10);
        struct listIterator *iter = listGetIterator(l, 0);
        for (i = 0; i < 10; i++) {
            int *n = listNodeValue(listNext(iter));
            test_cond("append to list equal", *n == i);
        }
        listRotate(l);
        listRotate(l);
        listRotate(l);
        iter = listGetIterator(l, 0);
        for (i = 0; i < 7; i++) {
            int *n = listNodeValue(listNext(iter));
            test_cond("list rotate", *n == i+3);
        }
        int *n = listNodeValue(listNext(iter));
        test_cond("list rotate", *n == 0);
        freeList(l);
        freeListIterator(iter);

        i = 1;
        l = createList();
        appendToListTail(l, &i);
        removeListNode(l, listFirst(l));
        test_cond("Delete element", listLength(l) == 0);

        l = createList();
        listSetDup(l, dupInt);
        listSetFree(l, freeInt);
        listSetMatch(l, matchInt);
        test_cond("list own value", isListOwnValue(l));
        for (i = 0; i < 10; i++) {
            int *n = wmalloc(sizeof(int));
            *n = i;
            insertToListHead(l, n);
            wfree(n);
        }
        test_cond("Append to list tail length", listLength(l) == 10);
        iter = listGetIterator(l, 1);
        for (i = 0; i < 10; i++) {
            int *n = listNodeValue(listNext(iter));
            test_cond("append to list equal", *n == i);
        }
        freeListIterator(iter);
        int d = 1;
        insertToListHead(l, &d);
        d = 2;
        int *store = listNodeValue(listFirst(l));
        test_cond("list own value test", listLength(l) == 11 &&
                 *store == 1);
        removeListNode(l, listFirst(l));
        test_cond("list own value remove test", listLength(l) == 10);
        struct listNode *s = searchListKey(l, &d);
        test_cond("searchListKey matched", *((int *)(s->value)) == 2);
        removeListNode(l, s);
        s = searchListKey(l, &d);
        test_cond("searchListKey not matched", s == NULL && listLength(l) == 9);
        freeList(l);

    }
    test_report();
}
#endif
