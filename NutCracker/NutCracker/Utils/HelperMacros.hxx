#define BIT(x)	(1 << x)

#ifdef GET_NUTCRACKER_EVENTCLASS_HELPER
#define EVENT_CLASS_TYPE(type) static consteval EventType GetStaticType() { return EventType::##type; }\
							  virtual EventType GetEventType() const override { return GetStaticType (); }\
							  virtual const char* GetName() const override { return #type; }
#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }
#endif

// use the power of lambdas to forward member function (ex. register it as callback)
#define NUTCRACKER_BIND_FUNCTION (fn) [this] (auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#define MIN(x,y) (x > y ? y :  x)
#define MAX(x,y) (x > y ? x :  y)
#define ABS(x)   (x > 0 ? x : -x)
#define MOD(x,y) (x - ((float(x) / y) * y))