#include <string.h>
#include <stdio.h>
#include "trace.h"

#ifdef COMPILE_ON_ROBOT
#include <ucos_ii.h>
static OS_CPU_SR cpu_sr;
#define LOCK() {OS_ENTER_CRITICAL();}
#define UNLOCK() {OS_EXIT_CRITICAL();}
#define TIMESTAMP() get_uptime()
#else
#define LOCK() {}
#define UNLOCK() {}
#define TIMESTAMP() 0
#endif


static trace_var_t *disabled_list_head;
static trace_var_t *enabled_list_head;


static void remove_from_list(trace_var_t *v, trace_var_t **list)
{
    trace_var_t *parent = *list;
    if (parent == v) {
        *list = (*list)->next;
    } else {
        while (parent != NULL && parent->next != v) {
            parent = parent->next;
        }
        if (parent != NULL) { // parent->next == v
            parent->next = parent->next->next;
        }
    }
}

static void add_to_list(trace_var_t *v, trace_var_t **list)
{
    v->next = *list;
    *list = v;
}

static trace_var_t *_lookup_list(trace_var_t *list, const char *name)
{
    trace_var_t *v = list;
    while (v != NULL) {
        if (strcmp(v->name, name) == 0)
            return v;
        v = v->next;
    }
    return NULL;
}

static trace_var_t *lookup(const char *name)
{
    trace_var_t *v;
    LOCK();
    v = _lookup_list(enabled_list_head, name);
    if (v != NULL)
        return v;
    v = _lookup_list(disabled_list_head, name);
    if (v != NULL)
        return v;
    UNLOCK();
    return NULL;
}

void trace_init(void)
{
    disabled_list_head = NULL;
    enabled_list_head = NULL;
}

void trace_var_add(trace_var_t *v, const char *name)
{
    v->trace_en = false;
    v->last_val = 0;
    v->name = name;
    LOCK();
    add_to_list(v, &disabled_list_head);
    UNLOCK();
}

void trace_var_delete(trace_var_t *v)
{
    LOCK();
    if (v->trace_en)
        remove_from_list(v, &enabled_list_head);
    else
        remove_from_list(v, &disabled_list_head);
    UNLOCK();
}

static void _trace_var_enable(trace_var_t *v)
{
    if (!v->trace_en) {
        remove_from_list(v, &disabled_list_head);
        add_to_list(v, &enabled_list_head);
        v->trace_en = true;
    }
}

void trace_var_enable(trace_var_t *v)
{
    LOCK();
    _trace_var_enable(v);
    UNLOCK();
}

static void _trace_var_disable(trace_var_t *v)
{
    if (v->trace_en) {
        remove_from_list(v, &enabled_list_head);
        add_to_list(v, &disabled_list_head);
        v->trace_en = false;
    }
}

void trace_var_disable(trace_var_t *v)
{
    LOCK();
    _trace_var_disable(v);
    UNLOCK();
}

void trace_var_enable_by_name(char *name)
{
    LOCK();
    trace_var_t *v = lookup(name);
    if (v != NULL)
        _trace_var_enable(v);
    UNLOCK();
}

void trace_var_disable_by_name(char *name)
{
    LOCK();
    trace_var_t *v = lookup(name);
    if (v != NULL)
        _trace_var_disable(v);
    UNLOCK();
}

void trace_var_disable_all(void)
{
    LOCK();
    trace_var_t *v = enabled_list_head;
    while (v != NULL) {
        _trace_var_disable(v);
        v = v->next;
    }
    UNLOCK();
}

void trace_var_update(trace_var_t *t, float val)
{
    t->last_val = val;
}

void trace_var_output(char *buffer, size_t buf_size)
{
    LOCK();
    trace_var_t *v = enabled_list_head;
    int remaining_sz = buf_size;
    char *buf = buffer;
    while (v != NULL) {
        int ret = snprintf(buf, remaining_sz, "%s %f\n", v->name, v->last_val);
        if (ret < remaining_sz) {
            buf += ret;
            remaining_sz -= ret;
        } else {
            *buf = '\0';
        }
        v = v->next;
    }
    UNLOCK();
}

