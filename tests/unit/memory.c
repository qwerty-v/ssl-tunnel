#include <ssl-tunnel/memory.h>

#include <stdio.h>
#include <assert.h>

void test_alloc() {
    scope *m;

    assert(mem_alloc_scope(&m) == ERROR_OK);

    mem_destroy_scope(m);
}

void test_init() {
    scope *m;

    assert(mem_alloc_scope(&m) == ERROR_OK);
    assert(scope_init(m) == ERROR_OK);
    assert(m->allocated_objs->len == 0);
    assert(m->allocated_objs->element_size == sizeof(object));

    mem_destroy_scope(m);
}

void test_scope_alloc() {
    scope *m;

    assert(mem_alloc_scope(&m) == ERROR_OK);
    assert(scope_init(m) == ERROR_OK);

    int *num = 0;
    assert(scope_alloc(m, (void *) &num, sizeof(int)) == ERROR_OK);
    assert(num != 0);

    *num = 5;

    object obj = ((object *)m->allocated_objs->elements)[0];
    assert(obj.ptr == num);
    assert(obj.size == sizeof(int));
    assert(*((int *)obj.ptr) == 5);

    mem_destroy_scope(m);
}

int main() {
    test_alloc();
    printf("OK test_alloc\n");
    test_init();
    printf("OK test_init\n");
    test_scope_alloc();
    printf("OK test_scope_alloc\n");
    return 0;
}