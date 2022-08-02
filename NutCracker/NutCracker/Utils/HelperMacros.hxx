#define BIT(x)	 (1 << x)

#ifdef GET_NUTCRACKER_EVENTCLASS_HELPER
#define EVENT_CLASS_TYPE(type)	static  consteval EventType GetStaticType ()	 { return EventType::##type; }\
								virtual constexpr EventType GetEventType  ()	 const override { return GetStaticType (); }\
								virtual constexpr const char* GetName ()		 const override { return #type; }
#define EVENT_CLASS_CATEGORY(category) virtual constexpr int GetCategoryFlags () const override { return category; }
#endif

#define	MIN(x,y)	(x > y ? y :  x)
#define	MAX(x,y)	(x > y ? x :  y)
#define	ABS(x)		(x > 0 ? x : -x)
#define	MOD(x,y)	(x - ((float (x) / y) * y))

#define EXPAND_MACRO(...) __VA_ARGS__

// detail macros for NTKR_VMACRO
#define __RSEQ_N()	9,8,7,6,5,4,3,2,1,0
#define __ARG_N(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,N,...)    N
#define __NARG__(...)			EXPAND_MACRO(__ARG_N(__VA_ARGS__))
#define __NARG_I(...)			__NARG__(##__VA_ARGS__, EXPAND_MACRO(__RSEQ_N()))
#define __CONCAT_DETAIL(a, b)	a##b
#define __VFUNC_f(name, N)		__CONCAT_DETAIL(name, N)   // expand
#if _MSC_VER
    #define __VCALL_NARG_I(...)       __NARG_I(##__VA_ARGS__)
    #define __NTKR_VMACRO(func,...)	 __VFUNC_f(func, __VCALL_NARG_I(, __VA_ARGS__)) (__VA_ARGS__)
#else
    #define __VCALL_NARG_I(...)       __NARG_I(,##__VA_ARGS__)
    #define __NTKR_VMACRO(func,...)	 __VFUNC_f(func, __VCALL_NARG_I(__VA_ARGS__)) (__VA_ARGS__)
#endif

#define NTKR_VMACRO(func,...)	__NTKR_VMACRO(func, __VA_ARGS__) // implemented as:  #define VK_ASSERT(...)	NTKR_VMACRO(VK_ASSERT_impl_, __VA_ARGS__) -> VK_ASSERT_impl_0, VK_ASSERT_impl_1, VK_ASSERT_impl_2 etc.

#define NTKR_BIND_FUNCTION_impl_2(handle,fn)  [handle] (auto&&... args)->decltype (auto) { return handle->fn (std::forward<decltype (args)> (args)...); }
#define NTKR_BIND_FUNCTION_impl_1(fn)          NTKR_BIND_FUNCTION_impl_2(this,fn)
#define NTKR_BIND_FUNCTION(...)	               NTKR_VMACRO(NTKR_BIND_FUNCTION_impl_, __VA_ARGS__)
// use the power of lambdas to forward member function (ex. register it as callback)