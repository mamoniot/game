//By Monica Moniot

#ifndef MAMLIB__H_INCLUDE
#define MAMLIB__H_INCLUDE

#ifdef __cplusplus
	#ifdef MAMLIB_STATIC
		#define MAMLIB__DECLR static
	#else
		#define MAMLIB__DECLR extern "C"
	#endif
#else
	#ifdef MAMLIB_STATIC
		#define MAMLIB__DECLR static
	#else
		#define MAMLIB__DECLR extern
	#endif
#endif
#define MAMLIB__DECLS static inline

#define MAMLIB_CHECK_COOKIE ((mam_int)0x4433443344334433ll)



#include "inttypes.h"
#define MAMLIB_BYTE int8_t
#if defined(_WIN64) || defined(__x86_64__) || defined(__ia64__) || defined(__LP64__)
	#define MAM_PTR64
#endif
#ifdef MAM_PTR64
	typedef char mamlib__testsize2_ptr[sizeof(char *) == 8];
	typedef int64_t mam_int;
#else
	typedef char mamlib__testsize2_ptr[sizeof(char *) == 4];
	typedef int32_t mam_int;
#endif
#define MAMLIB_ALIGNMENT sizeof(void*)

#include "stdlib.h"
#ifndef MAMLIB_MALLOC
	#define MAMLIB_MALLOC malloc
	#define MAMLIB_REALLOC realloc
	#define MAMLIB_FREE free
#endif
#include "string.h"
#ifndef MAMLIB_STRLEN
	#define MAMLIB_STRLEN strlen
#endif
#ifndef MAMLIB_MEMCPY
	#define MAMLIB_MEMCPY memcpy
#endif
#ifndef MAMLIB_MEMCMP
	#define MAMLIB_MEMCMP memcmp
#endif
#include "stdio.h"
#ifndef MAMLIB_PUTCHAR
	#define MAMLIB_PUTCHAR putchar
#endif
#include "stdarg.h"

typedef MAMLIB_BYTE mam_byte;



//////////////////////////////////////////////////////////////////////////////
//
//                         Strings
//

typedef struct MamString {
	char* ptr;
	mam_int size;
} MamString;
typedef struct MamString MamStr;

MAMLIB__DECLS MamString mam_nullstr() {
	MamString ret = {0};
	return ret;
}
MAMLIB__DECLS MamString mam_tostr(char* str) {
	MamString ret = {str};
	ret.size = MAMLIB_STRLEN(str);
	return ret;
}
MAMLIB__DECLS MamString mam_consttostr(const char* str) {
	MamString ret;
	ret.ptr = (char*)str;//lol
	ret.size = MAMLIB_STRLEN(str);
	return ret;
}
MAMLIB__DECLS MamString mam_memtostr(void* mem, mam_int mem_size) {
	MamString ret;
	ret.ptr = (char*)mem;
	ret.size = mem_size;
	return ret;
}

#define MAM_MAXINTDIGITS 20
MAMLIB__DECLR void mam__uinttostr(char* buffer, mam_int* i, mam_int n);
MAMLIB__DECLS MamString mam_uinttostr(char* buffer, mam_int n) {
	mam_int i = 0;
	mam__uinttostr(buffer, &i, n);
	MamString ret = {buffer, i};
	return ret;
}
MAMLIB__DECLS MamString mam_inttostr(char* buffer, mam_int n) {
	mam_int i;
	if(n < 0) {
		buffer[0] = '-';
		i = 1;
	} else {
		i = 0;
	}
	mam__uinttostr(buffer, &i, n);
	MamString ret = {buffer, i};
	return ret;
}
MAMLIB__DECLR void mam__hextostr(char* buffer, mam_int* i, mam_int n);
MAMLIB__DECLS MamString mam_hextostr(char* buffer, mam_int n) {
	mam_int i = 0;
	mam__hextostr(buffer, &i, n);
	MamString ret = {buffer, i};
	return ret;
}
MAMLIB__DECLS MamString mam_0xtostr(char* buffer, mam_int n) {
	buffer[0] = '0';
	buffer[1] = 'x';
	mam_int i = 2;
	mam__hextostr(buffer, &i, n);
	MamString ret = {buffer, i};
	return ret;
}

MAMLIB__DECLS MamString mam_substr(MamString str, mam_int i0, mam_int i1) {
	if(i0 < 0) i0 = 0;
	if((i1 < 0) | (i1 > str.size)) i1 = str.size;
	if((i0 >= str.size) | (i1 <= i0)) {
		return mam_nullstr();
	} else {
		mam_int new_size = i1 - i0;
		MamString ret = {str.ptr + i0, i1};
		return ret;
	}
}

MAMLIB__DECLS int mam_streq(MamString str0, MamString str1) {
	return str0.size == str1.size && MAMLIB_MEMCMP(str0.ptr, str1.ptr, str0.size) == 0;
}
MAMLIB__DECLS int mam_strcmp(MamString str0, MamString str1) {
	return str0.size > str1.size ? 1 : (str0.size < str1.size ? -1 : MAMLIB_MEMCMP(str0.ptr, str1.ptr, str0.size));
}
MAMLIB__DECLS int mam_cstreq(MamString str0, const char* str1) {
	return mam_streq(str0, mam_consttostr(str1));
}
MAMLIB__DECLS int mam_cstrcmp(MamString str0, const char* str1) {
	return mam_strcmp(str0, mam_consttostr(str1));
}

MAMLIB__DECLS MamString mam_strcpy(void* buffer, MamString str) {
	MAMLIB_MEMCPY(buffer, str.ptr, str.size);
	MamString ret = {(char*)buffer, str.size};
	return ret;
}
MAMLIB__DECLS MamString mam_strcat(void* buffer, MamString str0, MamString str1) {
	MAMLIB_MEMCPY(buffer, str0.ptr, str0.size);
	MAMLIB_MEMCPY((char*)buffer + str0.size, str1.ptr, str1.size);
	MamString ret = {(char*)buffer, str0.size + str1.size};
	return ret;
}

MAMLIB__DECLR int mam_strtouint64(MamString str, uint64_t* ret_n);
MAMLIB__DECLR int mam_strtouint32(MamString str, uint32_t* ret_n);
MAMLIB__DECLR int mam_strtoint64(MamString str, int64_t* ret_n);
MAMLIB__DECLR int mam_strtoint32(MamString str, int32_t* ret_n);
MAMLIB__DECLR int mam_strtohex64(MamString str, uint64_t* ret_n);
MAMLIB__DECLR int mam_strtohex32(MamString str, uint32_t* ret_n);
MAMLIB__DECLS int mam_strtonum64(MamString str, int64_t* ret_n) {
	if(str.ptr[0] == '0' && str.ptr[1] == 'x') {
		return mam_strtohex64(mam_substr(str, 2, -1), (uint64_t*)ret_n);
	} else {
		return mam_strtoint64(str, ret_n);
	}
}
MAMLIB__DECLS int mam_strtonum32(MamString str, int32_t* ret_n) {
	if(str.ptr[0] == '0' && str.ptr[1] == 'x') {
		return mam_strtohex32(mam_substr(str, 2, -1), (uint32_t*)ret_n);
	} else {
		return mam_strtoint32(str, ret_n);
	}
}


#ifdef __cplusplus
MAMLIB__DECLS MamString mam_tostr(const char* str) {return mam_consttostr(str);}
#endif

#ifdef MAMLIB_IMPLEMENTATION
int mam_strtoint64(MamString str, int64_t* ret_n) {
	int64_t n = 0;
	mam_int i;
	int sign;
	if(str.ptr[0] == '-') {
		i = 1;
		sign = -1;
	} else {
		i = 0;
		sign = 1;
	}
	for(; i < str.size; i += 1) {
		char ch = str.ptr[i];
		if(('0' <= ch) & (ch <= '9')) {
			int m = sign*(ch - '0');
			n = n*10 + m;
		} else {
			return 0;
		}
	}
	*ret_n = n;
	return 1;
}
int mam_strtoint32(MamString str, int32_t* ret_n) {
	int32_t n = 0;
	mam_int i;
	int sign;
	if(str.ptr[0] == '-') {
		i = 1;
		sign = -1;
	} else {
		i = 0;
		sign = 1;
	}
	for(; i < str.size; i += 1) {
		char ch = str.ptr[i];
		if(('0' <= ch) & (ch <= '9')) {
			n *= 10;
			n += sign*(ch - '0');
		} else {
			return 0;
		}
	}
	*ret_n = n;
	return 1;
}
int mam_strtouint64(MamString str, uint64_t* ret_n) {
	uint64_t n = 0;
	mam_int i;
	int sign;
	{
		i = 0;
		sign = 1;
	}
	for(; i < str.size; i += 1) {
		char ch = str.ptr[i];
		if(('0' <= ch) & (ch <= '9')) {
			n *= 10;
			n += sign*(ch - '0');
		} else {
			return 0;
		}
	}
	*ret_n = n;
	return 1;
}
int mam_strtouint32(MamString str, uint32_t* ret_n) {
	uint32_t n = 0;
	mam_int i;
	int sign;
	{
		i = 0;
		sign = 1;
	}
	for(; i < str.size; i += 1) {
		char ch = str.ptr[i];
		if(('0' <= ch) & (ch <= '9')) {
			n *= 10;
			n += sign*(ch - '0');
		} else {
			return 0;
		}
	}
	*ret_n = n;
	return 1;
}
int mam_strtohex64(MamString str, uint64_t* ret_n) {
	uint64_t n = 0;
	for(mam_int i = 0; i < str.size; i += 1) {
		char ch = str.ptr[i];
		n *= 16;
		if(('0' <= ch) & (ch <= '9')) {
			n += ch - '0';
		} else if(('a' <= ch) & (ch <= 'f')) {
			n += ch - 'a' + 10;
		} else if(('A' <= ch) & (ch <= 'F')) {
			n += ch - 'A' + 10;
		} else {
			return 0;
		}
	}
	*ret_n = n;
	return 1;
}
int mam_strtohex32(MamString str, uint32_t* ret_n) {
	uint32_t n = 0;
	for(mam_int i = 0; i < str.size; i += 1) {
		char ch = str.ptr[i];
		n *= 16;
		if(('0' <= ch) & (ch <= '9')) {
			n += ch - '0';
		} else if(('a' <= ch) & (ch <= 'f')) {
			n += ch - 'a' + 10;
		} else if(('A' <= ch) & (ch <= 'F')) {
			n += ch - 'A' + 10;
		} else {
			return 0;
		}
	}
	*ret_n = n;
	return 1;
}

MAMLIB__DECLR void mam__uinttostr(char* buffer, mam_int* i, mam_int n) {
	if(n > 10) mam__uinttostr(buffer, i, n/10);
	buffer[*i] = n%10 + '0';
	*i += 1;
}
MAMLIB__DECLR void mam__hextostr(char* buffer, mam_int* i, mam_int n) {
	if(n > 16) mam__hextostr(buffer, i, n/16);
	n %= 16;
	buffer[*i] = (n < 10) ? (n + '0') : (n + 'a');
	*i += 1;
}
#endif


//////////////////////////////////////////////////////////////////////////////
//
//                         Assertions
//

MAMLIB__DECLR void mam_error_str(const char* file, int line, MamString msg);

#if defined(MAMLIB_DEBUG) || defined(MAMLIB_FORCEASSERT)
	#define MAM_ASSERT(b) ((b) ? 0 : mam_error_str(__FILE__, __LINE__, mam_nullstr()))
	#define MAM_ASSERTL(b, msg) ((b) ? 0 : mam_error_str(__FILE__, __LINE__, mam_consttostr(msg)))
#else
	#define MAM_ASSERT(b) 0
	#define MAM_ASSERTL(b, msg) 0
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         General Allocator API
//

typedef enum MamAllocMode {
	MAM_MODE_ALLOC,
	MAM_MODE_REALLOC,
	MAM_MODE_FREE,
	MAM_MODE_FREEALL,
} MamAllocMode;

typedef void* MamAllocatorFunc(MamAllocMode mode, mam_int alloc_size, void* old_ptr, void* allocator_data);


#define mam_gen_malloc(allocator, allocator_data, alloc_size) ((allocator)(MAM_MODE_ALLOC, alloc_size, 0, allocator_data))
#define mam_gen_malloct(type, allocator, allocator_data, alloc_size) ((type*)((allocator)(MAM_MODE_ALLOC, alloc_size, 0, allocator_data)))
#define mam_gen_realloc(allocator, allocator_data, alloc_size, old_ptr) ((allocator)(MAM_MODE_REALLOC, alloc_size, old_ptr, allocator_data))
#define mam_gen_realloct(type, allocator, allocator_data, alloc_size, old_ptr) ((type*)((allocator)(MAM_MODE_REALLOC, alloc_size, old_ptr, allocator_data)))
#define mam_gen_free(allocator, allocator_data, old_ptr) ((allocator)(MAM_MODE_FREE, 0, old_ptr, allocator_data))
#define mam_gen_freeall(allocator, allocator_data) ((allocator)(MAM_MODE_FREEALL, 0, 0, allocator_data))



MAMLIB__DECLR void* mam__system_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, void* allocator_data);
#define mam_system_allocator mam_check_system_allocator


#ifdef MAMLIB_IMPLEMENTATION
void* mam__system_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, void* allocator_data) {
	if(mode == MAM_MODE_ALLOC) {
		return MAMLIB_MALLOC(alloc_size);
	} else if(mode == MAM_MODE_REALLOC) {
		return MAMLIB_REALLOC(old_ptr, alloc_size);
	} else if(mode == MAM_MODE_FREE) {
		MAMLIB_FREE(old_ptr);
	}
	return 0;
}
#endif


#define malloc(size) mam_gen_malloc(mam_system_allocator, 0, size)
#define realloc(ptr, size) mam_gen_realloc(mam_system_allocator, 0, size, ptr)
#define free(ptr) mam_gen_free(mam_system_allocator, 0, ptr)

//////////////////////////////////////////////////////////////////////////////
//
//                         Memory Checking
//

MAMLIB__DECLS mam_int mam_align(mam_int a) {
	//raises a to the nearest multiple of MAMLIB_ALIGNMENT
	return MAMLIB_ALIGNMENT*((a + (MAMLIB_ALIGNMENT - 1))/MAMLIB_ALIGNMENT);
}

#define mam_ptr_add(type, ptr, n) ((type*)((mam_byte*)(ptr) + (n)))
#define mam_ptr_sub(ptr0, ptr1) ((mam_byte*)(ptr0) - (mam_byte*)(ptr1))

MAMLIB__DECLR void* mam_check_allocation(MamAllocatorFunc* allocator, void* allocator_data, MamAllocMode mode, mam_int alloc_size, void* old_ptr);
MAMLIB__DECLR void* mam_check_system_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, void* allocator_data);


#ifdef MAMLIB_DEBUG
#define mam__underflow_msg "mamlib: invalid buffer detected; either an invalid pointer was given(use-after-free), a buffer underflow occured, or an overflow of an adjacent buffer occured\n"
#define mam__overflow_msg "mamlib: buffer overflow detected\n"

#define MAM_CHECK_BASE_SIZE (mam_align(2*sizeof(mam_int)))
#define MAM_CHECK_FULL_SIZE (MAM_CHECK_BASE_SIZE + sizeof(mam_int))

MAMLIB__DECLS void mam_check_mark(void* buffer, mam_int buffer_size) {
	*mam_ptr_add(mam_int, buffer, -(mam_int)sizeof(mam_int)) = MAMLIB_CHECK_COOKIE;
	*mam_ptr_add(mam_int, buffer, -(mam_int)2*sizeof(mam_int)) = buffer_size;
	*mam_ptr_add(mam_int, buffer, buffer_size) = MAMLIB_CHECK_COOKIE;
}
MAMLIB__DECLS void* mam_check_on_alloc(void* mem, mam_int buffer_size) {
	void* buffer = mam_ptr_add(void*, mem, MAM_CHECK_BASE_SIZE);
	mam_check_mark(buffer, buffer_size);
	return buffer;
}

/*******************************************************************************
/                         ATTENTION: DEBUGGER NOTICE
/*******************************************************************************
If your debugger takes you here because of an assertion, look at the assertion
message for more information, and walk the stack back up to your own code.
This library provides the following memory check functions to do quick and
dirty memory intergrity tests while the library is in debug mode.
They catch the most common bugs such as buffer overflows or use-after-free.
This works by placing memory cookies above and below every allocated buffer,
and checking their existance regularly.
The default cookie is 0x4433443344334433 == 4914346605369574451 == "3D3D3D3D"
********************************************************************************/
MAMLIB__DECLS void mam_check(void* buffer) {
	MAM_ASSERTL(*(mam_int*)((mam_byte*)buffer - sizeof(mam_int)) == MAMLIB_CHECK_COOKIE, mam__underflow_msg);
	mam_int buffer_size = *mam_ptr_add(mam_int, buffer, -(mam_int)2*sizeof(mam_int));
	MAM_ASSERTL(*(mam_int*)((mam_byte*)buffer + buffer_size) == MAMLIB_CHECK_COOKIE, mam__overflow_msg);
}
MAMLIB__DECLS void mam_check_unmark(void* buffer) {
	MAM_ASSERTL(*(mam_int*)((mam_byte*)buffer - sizeof(mam_int)) == MAMLIB_CHECK_COOKIE, mam__underflow_msg);
	mam_int buffer_size = *mam_ptr_add(mam_int, buffer, -(mam_int)2*sizeof(mam_int));
	MAM_ASSERTL(*(mam_int*)((mam_byte*)buffer + buffer_size) == MAMLIB_CHECK_COOKIE, mam__overflow_msg);
	*mam_ptr_add(mam_int, buffer, -(mam_int)sizeof(mam_int)) = 0;
	*mam_ptr_add(mam_int, buffer, buffer_size) = 0;
}
MAMLIB__DECLS void* mam_check_on_free(void* buffer) {
	MAM_ASSERTL(*(mam_int*)((mam_byte*)buffer - sizeof(mam_int)) == MAMLIB_CHECK_COOKIE, mam__underflow_msg);
	mam_int buffer_size = *mam_ptr_add(mam_int, buffer, -(mam_int)2*sizeof(mam_int));
	MAM_ASSERTL(*(mam_int*)((mam_byte*)buffer + buffer_size) == MAMLIB_CHECK_COOKIE, mam__overflow_msg);
	*mam_ptr_add(mam_int, buffer, -(mam_int)sizeof(mam_int)) = 0;
	*mam_ptr_add(mam_int, buffer, buffer_size) = 0;
	return mam_ptr_add(void, buffer, -MAM_CHECK_BASE_SIZE);
}



#ifdef MAMLIB_IMPLEMENTATION
void* mam_check_allocation(MamAllocatorFunc* allocator, void* allocator_data, MamAllocMode mode, mam_int alloc_size, void* old_ptr) {
	if(mode == MAM_MODE_ALLOC) {
		return mam_check_on_alloc(allocator(mode, alloc_size + MAM_CHECK_FULL_SIZE, old_ptr, allocator_data), alloc_size);
	} else if(mode == MAM_MODE_REALLOC) {
		if(alloc_size) {
			return mam_check_on_alloc(allocator(mode, alloc_size + MAM_CHECK_FULL_SIZE, mam_check_on_free(old_ptr), allocator_data), alloc_size);
		} else {
			return allocator(mode, alloc_size, mam_check_on_free(old_ptr), allocator_data);
		}
	} else if(mode == MAM_MODE_FREE) {
		return allocator(mode, alloc_size, mam_check_on_free(old_ptr), allocator_data);
	}
	return allocator(mode, alloc_size, old_ptr, allocator_data);
}
#endif
#else
#define MAM_CHECK_BASE_SIZE 0
#define MAM_CHECK_MARK_SIZE 0

#define mam_check_mark(buffer, buffer_size) 0
#define mam_check_alloc(mem, buffer_size) ((void*)(mem))

#define mam_check(buffer, buffer_size) 0

#define mam_check_unmark(buffer, buffer_size) 0

#ifdef MAMLIB_IMPLEMENTATION
void* mam_check_allocation(MamAllocatorFunc* allocator, void* allocator_data, MamAllocMode mode, mam_int alloc_size, void* old_ptr) {
	return allocator(mode, alloc_size, old_ptr, allocator_data);
}
#endif
#endif


#ifdef MAMLIB_IMPLEMENTATION
void* mam_check_system_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, void* allocator_data) {
	return mam_check_allocation(&mam__system_allocator, allocator_data, mode, alloc_size, old_ptr);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         Stack Allocator
//

typedef struct MamStack {
	mam_int size;
	mam_int highest_size;
	mam_int capacity;
} MamStack;


MAMLIB__DECLR MamStack* mam_stack_init(void* alloc_ptr, mam_int alloc_size);
MAMLIB__DECLR void* mam__stack_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, void* allocator_data);
MAMLIB__DECLR void* mam_stack_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, void* allocator_data);


MAMLIB__DECLS void* mam_stack_push(MamStack* stack, mam_int size) {
	return mam_gen_malloc(mam_stack_allocator, stack, size);
}
MAMLIB__DECLS void mam_stack_pop(MamStack* stack, void* ptr) {
	mam_gen_free(mam_stack_allocator, stack, ptr);
}
MAMLIB__DECLS void mam_stack_extend(MamStack* stack, void* ptr, mam_int size) {
	mam_gen_realloc(mam_stack_allocator, stack, size, ptr);
}
MAMLIB__DECLS void mam_stack_set_size(MamStack* stack, mam_int new_size) {
	stack->size = new_size;
	if(stack->highest_size < new_size) stack->highest_size = new_size;
	MAM_ASSERTL(new_size <= stack->capacity, "mamlib: stack is out of memory");
}
#define mam_stack_pusht(type, stack, size) ((type*)mam_stack_push(stack, sizeof(type)*(size)))


#ifdef MAMLIB_IMPLEMENTATION
MamStack* mam_stack_init(void* alloc_ptr, mam_int alloc_size) {
	MamStack* stack = (MamStack*)alloc_ptr;
	stack->size = 0;
	stack->highest_size = 0;
	stack->capacity = alloc_size - sizeof(MamStack);
	return stack;
}

void* mam__stack_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, void* allocator_data) {
	MamStack* stack = (MamStack*)allocator_data;
	if(mode == MAM_MODE_ALLOC) {
		alloc_size = mam_align(alloc_size);
		mam_int new_size = stack->size + alloc_size;

		void* mem = mam_ptr_add(void, stack, sizeof(MamStack) + stack->size);
		stack->size = new_size;
		if(stack->highest_size < new_size) stack->highest_size = new_size;
		if(new_size > stack->capacity) {
			MAM_ASSERTL(new_size > stack->capacity, "mamlib: stack is out of memory");
			mem = MAMLIB_MALLOC(alloc_size);
		}

		return mem;
	} else if(mode == MAM_MODE_REALLOC && alloc_size) {
		mam_int new_size = mam_ptr_sub(old_ptr, stack) - sizeof(MamStack);
		MAM_ASSERTL(new_size > 0, "mamlib: attempt to pop stack when stack is empty");

		alloc_size = mam_align(alloc_size);
		new_size += alloc_size;

		void* mem = mam_ptr_add(void, stack, sizeof(MamStack) + stack->size);
		stack->size = new_size;
		if(stack->highest_size < new_size) stack->highest_size = new_size;
		if(new_size > stack->capacity) {
			MAM_ASSERTL(new_size > stack->capacity, "mamlib: stack is out of memory");
			mem = MAMLIB_MALLOC(alloc_size);
		}
		return mem;
	} else if(mode == MAM_MODE_FREE || (mode == MAM_MODE_REALLOC && !alloc_size)) {
		mam_int new_size = mam_ptr_sub(old_ptr, stack) - sizeof(MamStack);

		stack->size = new_size;
		MAM_ASSERTL(new_size > 0, "mamlib: attempt to pop stack when stack is empty");
	} else if(mode == MAM_MODE_FREEALL) {
		stack->size = 0;
	}
	return 0;
}
void* mam_stack_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, void* allocator_data) {
	return mam_check_allocation(&mam__stack_allocator, allocator_data, mode, alloc_size, old_ptr);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         General Logging API
//

typedef void MamLoggerFunc(MamString str, void* logger_data);

MAMLIB__DECLR void mam__system_logger(MamString str, void* logger_data);
#define mam_system_logger mam__system_logger

#ifdef MAMLIB_IMPLEMENTATION
void mam__system_logger(MamString str, void* logger_data) {
	for(mam_int i = 0; i < str.size; i += 1) MAMLIB_PUTCHAR(str.ptr[i]);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         Error Handling
//

typedef void MamTrapFunc(void* trap_data);

MAMLIB__DECLR void mam__system_error_trap(void* trap_data) {
	#ifdef MAMLIB_DEBUG
	    #ifdef _MSC_VER
		    #ifndef MAM_PTR64
		    	__asm int 3;   // trap to debugger!
		    #else
		    	__debugbreak();
		    #endif
	    #else
	    	__builtin_trap();
	    #endif
    #endif
	exit(1);
}
#define mam_system_error_trap mam__system_error_trap

MAMLIB__DECLR void mam_set_assertion_logger(MamLoggerFunc* logger, void* logger_data);
MAMLIB__DECLR void mam_set_error_trap(MamTrapFunc* trap);


MAMLIB__DECLR void mam_error_str(const char* file, int line, MamString msg);
MAMLIB__DECLS void mam_error(const char* file, int line) {
	mam_error_str(file, line, mam_nullstr());
}
MAMLIB__DECLS void mam_error_cstr(const char* file, int line, const char* msg) {
	mam_error_str(file, line, mam_consttostr(msg));
}

#define MAM_ERROR() mam_error(__FILE__, __LINE__)
#define MAM_ERRORL(msg) mam_error_cstr(__FILE__, __LINE__, msg)
//moved to top of file for general usage
// #if defined(MAMLIB_DEBUG) || defined(MAMLIB_FORCEASSERT)
// 	#define MAM_ASSERT(b) ((b) ? 0 : mam_error(__FILE__, __LINE__))
// 	#define MAM_ASSERTL(b, msg) ((b) ? 0 : mam_error_cstr(__FILE__, __LINE__, msg))
// #else
// 	#define MAM_ASSERT(b) 0
// 	#define MAM_ASSERTL(b, msg) 0
// #endif

#ifndef MAM_LONGNAMES_ONLY
#define ERROR MAM_ERROR
#define ERRORL MAM_ERRORL
#define ASSERT MAM_ASSERT
#define ASSERTL MAM_ASSERTL
#endif


#ifdef MAMLIB_IMPLEMENTATION
static MamLoggerFunc* mam__error_logger = &mam_system_logger;
static void* mam__error_logger_data = 0;
static MamTrapFunc* mam__error_trap = &mam_system_error_trap;
static void* mam__trap_data = 0;

void mam_set_assertion_logger(MamLoggerFunc* logger, void* logger_data) {
	mam__error_logger = logger;
	mam__error_logger_data = logger_data;
}
void mam_set_error_trap(MamTrapFunc* trap, void* trap_data) {
	mam__error_trap = trap;
	mam__trap_data = trap_data;
}
void mam_error_str(const char* file, int line, MamString msg) {
	char buffer[MAM_MAXINTDIGITS];
	mam__error_logger(mam_consttostr("["), mam__error_logger_data);
	mam__error_logger(mam_consttostr(file), mam__error_logger_data);
	mam__error_logger(mam_consttostr(":"), mam__error_logger_data);
	mam__error_logger(mam_uinttostr(buffer, line), mam__error_logger_data);
	if(msg.size) {
		mam__error_logger(mam_consttostr("] Fatal error thrown: "), mam__error_logger_data);
		mam__error_logger(msg, mam__error_logger_data);
	} else {
		mam__error_logger(mam_consttostr("] Fatal error thrown\n"), mam__error_logger_data);
	}
	mam__error_trap(mam__trap_data);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         printf
//

#ifdef MAMLIB_STB_SPRINTF

#ifdef MAMLIB_IMPLEMENTATION
	#define STB_SPRINTF_IMPLEMENTATION
#endif
#include "stb_sprintf.h"

MAMLIB__DECLR int mam_logf(MamLoggerFunc* logger, void* logger_data, const char* fmt, ...);
MAMLIB__DECLR int mam_vlogf(MamLoggerFunc* logger, void* logger_data, const char* fmt, va_list va);
MAMLIB__DECLS int mam_printf(const char* fmt, ...) {
	int ret;
	va_list va;
	va_start(va, fmt);
	ret = mam_vlogf(&mam_system_logger, 0, fmt, va);
	va_end(va);
	return ret;
}


#ifndef MAM_LONGNAMES_ONLY
	#define printf mam_printf
	#define sprintf stbsp_sprintf
	#define vsprintf stbsp_vsprintf
	#define vsnprintf stbsp_vsnprintf
#endif


#ifdef MAMLIB_IMPLEMENTATION
struct mam__printf_userdata {
	MamLoggerFunc* logger;
	void* logger_data;
	char buffer[STB_SPRINTF_MIN];
};
char* mam__logf_cb(char const* buffer, void* userdata, int buffer_size) {
	struct mam__printf_userdata* data = ((struct mam__printf_userdata*)userdata);
	MamString str = {(char*)buffer, (mam_int)buffer_size};
	data->logger(str, data->logger_data);
	return data->buffer;
}
int mam_vlogf(MamLoggerFunc* logger, void* logger_data, const char* fmt, va_list va) {
	struct mam__printf_userdata userdata = {logger, logger_data};
	return stbsp_vsprintfcb(&mam__logf_cb, &userdata, userdata.buffer, fmt, va);
}
int mam_logf(MamLoggerFunc* logger, void* logger_data, const char* fmt, ...) {
	int ret;
	va_list va;
	va_start(va, fmt);
	ret = mam_vlogf(logger, logger_data, fmt, va);
	va_end(va);
	return ret;
}
#endif

#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         Fast String Builder
//

#define MAM_BUILDER_BUFFER_CAPACITY (1024 - 2*sizeof(void*))
typedef struct MamBuilderBuffer {
	struct MamBuilderBuffer* next;
	mam_int size;
	mam_byte mem[MAM_BUILDER_BUFFER_CAPACITY];
} MamBuilderBuffer;

typedef struct MamBuilder {
	mam_int size;
	mam_int failed;
	MamAllocatorFunc* allocator;
	void* allocator_data;
	MamBuilderBuffer head;
	MamBuilderBuffer* tail;
} MamBuilder;

MAMLIB__DECLR void mam_builder_init(MamBuilder* builder);
MAMLIB__DECLR void mam_builder_delete(MamBuilder* builder);
MAMLIB__DECLR void mam_builder_push_char(MamBuilder* builder, char ch);
MAMLIB__DECLR void mam_builder_push_mem(MamBuilder* builder, const void* mem, mam_int mem_size);
MAMLIB__DECLS void mam_builder_push_str(MamBuilder* builder, MamString str) {
	mam_builder_push_mem(builder, str.ptr, str.size);
}
MAMLIB__DECLS void mam_builder_push_cstr(MamBuilder* builder, const char* str) {
	mam_builder_push_str(builder, mam_consttostr(str));
}
MAMLIB__DECLS void mam_builder_push_uint(MamBuilder* builder, mam_int n) {
	char buffer[MAM_MAXINTDIGITS];
	mam_builder_push_str(builder, mam_uinttostr(buffer, n));
}
MAMLIB__DECLS void mam_builder_push_int(MamBuilder* builder, mam_int n) {
	char buffer[MAM_MAXINTDIGITS];
	mam_builder_push_str(builder, mam_inttostr(buffer, n));
}
MAMLIB__DECLS void mam_builder_push_hex(MamBuilder* builder, mam_int n) {
	char buffer[MAM_MAXINTDIGITS];
	mam_builder_push_str(builder, mam_hextostr(buffer, n));
}
MAMLIB__DECLS void mam_builder_push_0x(MamBuilder* builder, mam_int n) {
	char buffer[MAM_MAXINTDIGITS];
	mam_builder_push_str(builder, mam_0xtostr(buffer, n));
}

#ifdef MAMLIB_STB_SPRINTF
	MAMLIB__DECLR int mam_builder_pushf(MamBuilder* builder, const char* fmt, ...);
#endif


MAMLIB__DECLR void mam_builder_log(MamBuilder* builder, MamLoggerFunc* logger, void* logger_data);
MAMLIB__DECLS void mam_builder_print(MamBuilder* builder) {
	mam_builder_log(builder, &mam_system_logger, 0);
}

MAMLIB__DECLR MamString mam_builder_copytogen(MamBuilder* builder, MamAllocatorFunc* allocator, void* allocator_data);
MAMLIB__DECLR void mam_builder_copytomem(MamBuilder* builder, char* mem, mam_int mem_size);
MAMLIB__DECLS MamString mam_builder_copy(MamBuilder* builder) {
	return mam_builder_copytogen(builder, &mam_system_allocator, 0);
}

MAMLIB__DECLR void mam_builder_logger(MamString str, void* builder_ptr);


#ifdef MAMLIB_IMPLEMENTATION
void mam_builder_init(MamBuilder* builder) {
	builder->head.next = 0;
	builder->head.size = 0;
	builder->tail = &builder->head;
	builder->allocator = &mam_system_allocator;
	builder->allocator_data = 0;
	builder->failed = 0;
}
void mam_builder_delete(MamBuilder* builder) {
	MamBuilderBuffer* head = &builder->head;
	while(head) {
		mam_gen_free(builder->allocator, builder->allocator_data, head);
		head = head->next;
	}
}

void mam_builder_push_char(MamBuilder* builder, char ch) {
	MamBuilderBuffer* tail = builder->tail;
	if(tail->size >= MAM_BUILDER_BUFFER_CAPACITY) {
		MamBuilderBuffer* new_tail = mam_gen_malloct(MamBuilderBuffer, builder->allocator, builder->allocator_data, 1);
		if(!new_tail) {builder->failed = 1; return;}
		new_tail->size = 0;
		new_tail->next = 0;
		tail->next = new_tail;
		tail = new_tail;
	}
	tail->mem[tail->size] = ch;
	tail->size += 1;
	builder->size += 1;
}
void mam_builder_push_mem(MamBuilder* builder, const void* mem, mam_int mem_size) {
	MamBuilderBuffer* tail = builder->tail;
	while(1) {
		mam_int size_left = MAM_BUILDER_BUFFER_CAPACITY - tail->size;
		if(mem_size > size_left) {
			MAMLIB_MEMCPY(&tail->mem[tail->size], mem, size_left);
			tail->size += size_left;
			builder->size += size_left;
			mem_size -= size_left;
			mem = mam_ptr_add(void, mem, size_left);

			MamBuilderBuffer* new_tail = mam_gen_malloct(MamBuilderBuffer, builder->allocator, builder->allocator_data, 1);
			if(!new_tail) {builder->failed = 1; return;}
			new_tail->size = 0;
			new_tail->next = 0;
			tail->next = new_tail;
			tail = new_tail;
		} else {
			MAMLIB_MEMCPY(&tail->mem[tail->size], mem, mem_size);
			tail->size += mem_size;
			return;
		}
	}
}

#ifdef MAMLIB_STB_SPRINTF
	struct mam__builder_userdata {
		MamBuilder* builder;
		char buffer[STB_SPRINTF_MIN];
	};
	MAMLIB__DECLR char* mam__builder_pushf_cb(char const* buffer, void* userdata, int buffer_size) {
		mam_builder_push_mem((MamBuilder*)userdata, buffer, buffer_size);
		return ((struct mam__builder_userdata*)userdata)->buffer;
	}
	MAMLIB__DECLR int mam_builder_pushf(MamBuilder* builder, const char* fmt, ...) {
		struct mam__builder_userdata userdata = {builder};
		int ret;
		va_list va;
		va_start(va, fmt);
		ret = stbsp_vsprintfcb(&mam__builder_pushf_cb, &userdata, userdata.buffer, fmt, va);
		va_end(va);
		return ret;
	}
#endif
#endif

void mam_builder_log(MamBuilder* builder, MamLoggerFunc* logger, void* logger_data) {
	MamBuilderBuffer* head = &builder->head;
	while(head) {
		MamString str = {(char*)head->mem, head->size};
		logger(str, logger_data);
		head = head->next;
	}
}

void mam_builder_copytomem(MamBuilder* builder, char* mem, mam_int mem_size) {
	MamBuilderBuffer* head = &builder->head;
	while(head) {
		if(mem_size < head->size) {
			MAMLIB_MEMCPY(mem, head->mem, mem_size);
			break;
		} else {
			MAMLIB_MEMCPY(mem, head->mem, head->size);
			mem += head->size;
			mem_size -= head->size;
			head = head->next;
		}
	}
}
MamString mam_builder_copytogen(MamBuilder* builder, MamAllocatorFunc* allocator, void* allocator_data) {
	MamString str = {mam_gen_malloct(char, allocator, allocator_data, builder->size), builder->size};
	if(!str.ptr) return str;
	MamBuilderBuffer* head = &builder->head;
	while(head) {
		MAMLIB_MEMCPY(str.ptr, head->mem, head->size);
		head = head->next;
	}
	return str;
}

void mam_builder_logger(MamString str, void* builder_ptr) {
	mam_builder_push_mem((MamBuilder*)builder_ptr, str.ptr, str.size);
}

#ifdef MAMLIB_IMPLEMENTATION
#undef MAMLIB_IMPLEMENTATION
#endif

#endif

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Monica Moniot
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
