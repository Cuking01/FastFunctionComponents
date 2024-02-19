#include<stdio.h>
#include<type_traits>
#include<utility>
#include<vector>
#include<cmath>


struct Base;
struct Base_X;
struct Base_Y;
struct Base_Z;

template<typename T,typename R,typename...Args>
struct Interface
{
	T*t;
	R (T::*fun)(Base*,Args...);

	R operator()(Base*b,Args...args)
	{
		return (t->*fun)(b,std::forward<Args>(args)...);
	}
};

struct Base
{
	int cnt;
	Interface<Base_X,double,double> X;
	Interface<Base_Y,double,double> Y;
	Interface<Base_Z,double,double> Z;

	double calc_X(double t){return X(this,t);}
	double calc_Y(double t){return Y(this,t);}
	double calc_Z(double t){return Z(this,t);}

	virtual ~Base()=default;
};

struct Base_X
{
	Base_X(){}
	explicit Base_X(Base*b,Base_X attr)
	{
		b->X.t=this;
		b->X.fun=&calc;
		*this=attr;
	}

	double calc(Base*b,double t)
	{
		return t+1;
	}
};

struct Base_Y
{
	Base_Y(){}
	explicit Base_Y(Base*b,Base_Y attr)
	{
		b->Y.t=this;
		b->Y.fun=&calc;
		*this=attr;
	}
	double calc(Base*b,double t)
	{
		return t-1;
	}
};


struct Base_Z
{
	Base_Z(){}
	explicit Base_Z(Base*b,Base_Z attr)
	{
		b->Z.t=this;
		b->Z.fun=&calc;
		*this=attr;
	}
	double calc(Base*b,double t)
	{
		return t*2;
	}
};

template<bool x,typename T1,typename T2>
struct IF_impl
{
	using type=T1;
};

template<typename T1,typename T2>
struct IF_impl<false,T1,T2>
{
	using type=T2;
};

template<bool x,typename T1,typename T2>
using IF=typename IF_impl<x,T1,T2>::type;

template<typename T,typename...Ts>
struct select_T_impl;

template<typename T,typename T1,typename...Ts>
struct select_T_impl<T,T1,Ts...>
{
	using type=IF<std::is_base_of_v<T,std::remove_reference_t<T1>>,T1,typename select_T_impl<T,Ts...>::type>;
};

template<typename T>
struct select_T_impl<T>
{
	using type=T;
};

template<typename T,typename...Ts>
using select_T=typename select_T_impl<T,Ts...>::type;

template<typename T,typename...Args>
struct select_impl;

template<typename T,typename Arg1,typename...Args>
struct select_impl<T,Arg1,Args...>
{
	static constexpr bool matched=std::is_base_of_v<T,std::remove_reference_t<Arg1>>;
	using RT=IF<matched,Arg1&&,typename select_impl<T,Args...>::RT>;
	static RT impl(Arg1&&arg1,Args&&... args)
	{
		if constexpr(matched)
			return std::forward<Arg1>(arg1);
		else
			return select_impl<T,Args...>::impl(std::forward<Args>(args)...);
	}
};

template<typename T>
struct select_impl<T>
{
	using RT=T;
	static T impl()
	{
		return T{};
	}
};

template<typename T,typename...Args>
typename select_impl<T,Args...>::RT select(Args&&... args)
{
	return select_impl<T,Args...>::impl(std::forward<Args>(args)...);
}

template<typename... Bases>
struct Derived:Base,Bases...
{
	template<typename... Bases2>
	Derived(Bases2&&...bases2):
		Base(),
		Bases(static_cast<Base*>(this),select<Bases,Bases2...>(std::forward<Bases2>(bases2)...))...
	{}
};

template<typename... Ts>
Derived(Ts...)->Derived<select_T<Base_X,Ts...>,select_T<Base_Y,Ts...>,select_T<Base_Z,Ts...>>;

struct Poly_X:Base_X
{
	std::vector<double> a;
	explicit Poly_X(std::initializer_list<double> list):a{list}{}
	explicit Poly_X(Base*b,Poly_X&&attr):a(std::move(attr.a))
	{
		b->X.t=this;
		b->X.fun=static_cast<double(Base_X::*)(Base*,double)>(&calc);

	}

	double calc(Base*b,double t)
	{
		double res=0;
		double pt=1;
		for(auto ai:a)
		{
			res+=ai*pt;
			pt*=t;
		}
		return res;
	}

};

struct Log_Y:Base_Y
{
	double xi;
	explicit Log_Y(double di)
	{
		xi=1/std::log(di);
	}
	explicit Log_Y(Base*b,Log_Y attr):Log_Y(attr)
	{
		b->Y.t=this;
		b->Y.fun=static_cast<double(Base_Y::*)(Base*,double)>(&calc);
	}
	double calc(Base*b,double t)
	{
		return std::log(1+t)*xi;
	}
};

#include<array>

int main()
{
	std::array<std::pair<int,int>,2> tmp{1,2,3,4};

	Derived a(Base_X{});
	Derived b(Base_Y{},Poly_X{1.0,1.0,0.5});
	Derived c(Log_Y(5.0),Base_Z{},Poly_X{1.0});

	std::vector<Base*> points{(Base*)&a,(Base*)&b,(Base*)&c};

	for(int i=0;i<100;i++)
	{
		printf("t=%d\n",i);
		for(auto point:points)
		{
			printf("(%8.2f,%8.2f,%8.2f)\n",point->calc_X(i),point->calc_Y(i),point->calc_Z(i));
		}
		
	}
}
