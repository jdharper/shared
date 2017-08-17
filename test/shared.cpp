#include <vector>
#include "cppTest/cppTest.h"
#include "shared.h"

class A
{
public:
	static size_t na;
	A(int a) : a(a) { ++na;  }
	A(const A & a) : a(a.a) { ++na; }
	~A() { --na;  }
	int a;
};
size_t A::na = 0;

class B : public A
{
public:
	static size_t nb;
	B(int a, int b) : A(a), b(b) { ++nb; }
	B(const B & b) : A(b.a), b(b.b) { ++nb;  }
	~B() { --nb; }
	int b;
};
size_t B::nb = 0;

class C : public B, public intrusive_ref_counter
{
public:
	C(int a, int b) : B(a, b) {}
};

DEF_GROUP(intrusive_ptr);
DEF_TEST(intrusive_ptr, deallocate)
{
	CHECK_EXPECT(B::nb, 0);
	CHECK_EXPECT(A::na, 0);
	{
		intrusive_ptr<C> pb(new C(10, 20));
		CHECK_EXPECT(B::nb, 1);
		CHECK_EXPECT(A::na, 1);
	}
	CHECK_EXPECT(B::nb, 0);
	CHECK_EXPECT(A::na, 0);
}


class D : public shared_base<D>, public B
{
public:
	D(int a, int b) : B(a, b) {}
	D(const D & d) : B(d) {}
};

DEF_GROUP(shared_base);
DEF_TEST(shared_base, allocate)
{
	auto p = D::allocate(10, 20);
	CHECK_EXPECT(p->a, 10);
	CHECK_EXPECT((*p).b, 20);
	CHECK_EXPECT(B::nb, 1);

	D::ptr p2(p);
	CHECK_EXPECT(p2->a, 10);
	CHECK_EXPECT((*p2).b, 20);
	CHECK_EXPECT(B::nb, 1);
}

DEF_TEST(shared_base, no_copy_unique)
{
	auto p = D::allocate(10, 20);
	CHECK_EXPECT(p->a, 10);
	CHECK_EXPECT((*p).b, 20);
	CHECK_EXPECT(B::nb, 1);

	auto pw = p.write();
	CHECK_EXPECT(B::nb, 1);
	pw->a = 11;
	pw->b = 22;
	CHECK_EXPECT(pw->a, 11);
	CHECK_EXPECT(pw->b, 22);
	CHECK_EXPECT(p->a, 11);
	CHECK_EXPECT(p->b, 22);
}

DEF_TEST(shared_base, copy_on_write)
{
	auto p = D::allocate(10, 20);
	CHECK_EXPECT(p->a, 10);
	CHECK_EXPECT((*p).b, 20);
	CHECK_EXPECT(B::nb, 1);

	D::ptr p2(p);
	CHECK_EXPECT(p2->a, 10);
	CHECK_EXPECT((*p2).b, 20);
	CHECK_EXPECT(B::nb, 1);

	auto pw = p2.write();
	CHECK_EXPECT(B::nb, 2);
	pw->a = 11;
	pw->b = 22;

	CHECK_EXPECT(pw->a, 11);
	CHECK_EXPECT(pw->b, 22);
	CHECK_EXPECT(p2->a, 11);
	CHECK_EXPECT(p2->b, 22);
	CHECK_EXPECT(p->a, 10);
	CHECK_EXPECT(p->b, 20);

}

DEF_TEST(shared_base, no_copy_for_shared)
{
	auto p = D::allocate(10, 20).view();
	CHECK_EXPECT(p->a, 10);
	CHECK_EXPECT((*p).b, 20);
	CHECK_EXPECT(B::nb, 1);

	D::view_ptr p2(p);
	CHECK_EXPECT(p2->a, 10);
	CHECK_EXPECT((*p2).b, 20);
	CHECK_EXPECT(B::nb, 1);

	auto p3 = p.view();
	CHECK_EXPECT(p3->a, 10);
	CHECK_EXPECT((*p3).b, 20);
	CHECK_EXPECT(B::nb, 1);

	auto pw = p2.write();
	CHECK_EXPECT(B::nb, 1);
	pw->a = 11;
	pw->b = 22;

	CHECK_EXPECT(pw->a, 11);
	CHECK_EXPECT(pw->b, 22);
	CHECK_EXPECT(p2->a, 11);
	CHECK_EXPECT(p2->b, 22);
	CHECK_EXPECT(p3->a, 11);
	CHECK_EXPECT(p3->b, 22);
	CHECK_EXPECT(p->a, 11);
	CHECK_EXPECT(p->b, 22);
}

DEF_GROUP(shared);
DEF_TEST(shared, allocate)
{
	auto p = shared<int>::allocate(7);
	auto p2 = shared<int>::ptr(p);
	CHECK_EXPECT(*p, 7);
	CHECK_EXPECT(*p2, 7);

	auto pb = shared<B>::allocate(10, 20);
	CHECK_EXPECT(pb->a, 10);
	CHECK_EXPECT(pb->b, 20);
}

DEF_TEST(shared, shared_object)
{
	auto pb = shared<B>::allocate(10, 20);
	CHECK_EXPECT(pb->a, 10);
	CHECK_EXPECT(pb->b, 20);
	CHECK_EXPECT(B::nb, 1);
	CHECK_EXPECT(A::na, 1);

	auto pb2 = pb;
	CHECK_EXPECT(B::nb, 1);
	CHECK_EXPECT(A::na, 1);
	CHECK_EXPECT(pb2->a, 10);
	CHECK_EXPECT(pb2->b, 20);

	CHECK_EXPECT(pb2.get(), pb.get());

	shared<B>::ptr pb3;
	pb3 = pb;
	CHECK_EXPECT(B::nb, 1);
	CHECK_EXPECT(A::na, 1);
	CHECK_EXPECT(pb3->a, 10);
	CHECK_EXPECT(pb3->b, 20);

	CHECK_EXPECT(pb3.get(), pb.get());
}

DEF_TEST(shared, cow_shared)
{
	auto pb = shared<B>::allocate(10, 20).view();
	auto pb2 = pb.view();

	CHECK_EXPECT(pb2->a, 10);
	CHECK_EXPECT(pb2->b, 20);
	CHECK_EXPECT(B::nb, 1);


	{
		auto pbw = pb2.write();
		CHECK_EXPECT(B::nb, 1);
		CHECK_EXPECT(pbw->a, 10);
		CHECK_EXPECT(pbw->b, 20);
		pbw->a = 11;
		pbw->b = 22;
	}

	CHECK_EXPECT(pb->a, 11);
	CHECK_EXPECT(pb->b, 22);
	CHECK_EXPECT(pb2->a, 11);
	CHECK_EXPECT(pb2->b, 22);

		auto pb3 = pb2.cow();
		auto pb4 = pb3.cow();
		CHECK_EXPECT(B::nb, 1);
		{
			CHECK_EXPECT(pb3.get(), pb2.get());
			auto pbw = pb3.write();
			CHECK(pb3.get() != pb2.get());
			CHECK_EXPECT(pb3.get(), pbw);
			CHECK_EXPECT(B::nb, 2);
			CHECK_EXPECT(pbw->a, 11);
			CHECK_EXPECT(pbw->b, 22);
			pbw->a = 12;
			pbw->b = 24;
			CHECK_EXPECT(pbw->a, 12);
			CHECK_EXPECT(pbw->b, 24);
			CHECK_EXPECT(pb4->a, 11);
			CHECK_EXPECT(pb4->b, 22);
			CHECK_EXPECT(pb4.get(), pb.get());
		}
	
		CHECK_EXPECT(pb->a, 11);
		CHECK_EXPECT(pb->b, 22);
		CHECK_EXPECT(pb2->a, 11);
		CHECK_EXPECT(pb2->b, 22);
		CHECK_EXPECT(pb3->a, 12);
		CHECK_EXPECT(pb3->b, 24);

}


DEF_TEST(shared, cow)
{
	auto pb = shared<B>::allocate(10, 20);
	auto pb2 = shared<B>::ptr(pb);

	CHECK_EXPECT(pb2->a, 10);
	CHECK_EXPECT(pb2->b, 20);
	CHECK_EXPECT(B::nb, 1);


	{
		auto pbw = pb2.write();
		CHECK_EXPECT(B::nb, 2);
		CHECK_EXPECT(pbw->a, 10);
		CHECK_EXPECT(pbw->b, 20);
		pbw->a = 11;
		pbw->b = 22;
		CHECK_EXPECT(pbw->a, 11);
		CHECK_EXPECT(pbw->b, 22);
	}

	CHECK_EXPECT(pb->a, 10);
	CHECK_EXPECT(pb->b, 20);
	CHECK_EXPECT(pb2->a, 11);
	CHECK_EXPECT(pb2->b, 22);
}

DEF_TEST(shared, unitialized_ptr)
{
	shared<B>::ptr p1;
	shared<B>::view_ptr p2;
	CHECK(p1.get() ==  nullptr);
	CHECK(p2.get() ==  nullptr);
}

DEF_TEST(shared, new_group)
{
	auto p1 = shared<B>::allocate(10, 20).view();
	auto p2 = p1.view();
	auto p3 = p2.view(true);
	auto p4 = p3.view();

	CHECK_EXPECT(B::nb, 1);

	CHECK_EXPECT(p1->a, 10);
	CHECK_EXPECT(p1->b, 20);
	CHECK_EXPECT(p2->a, 10);
	CHECK_EXPECT(p2->b, 20);
	CHECK_EXPECT(p3->a, 10);
	CHECK_EXPECT(p3->b, 20);
	CHECK_EXPECT(p4->a, 10);
	CHECK_EXPECT(p4->b, 20);

	CHECK(p1.get() == p2.get());
	CHECK(p3.get() == p4.get());
	CHECK(p2.get() == p3.get());

	auto pw = p3.write();
	CHECK_EXPECT(B::nb, 2);
	CHECK(p1.get() == p2.get());
	CHECK(p3.get() == p4.get());
	CHECK(p2.get() != p3.get());
	pw->a = 11;
	pw->b = 22;
	CHECK_EXPECT(p1->a, 10);
	CHECK_EXPECT(p1->b, 20);
	CHECK_EXPECT(p2->a, 10);
	CHECK_EXPECT(p2->b, 20);
	CHECK_EXPECT(p3->a, 11);
	CHECK_EXPECT(p3->b, 22);
	CHECK_EXPECT(p4->a, 11);
	CHECK_EXPECT(p4->b, 22);

}

DEF_TEST(shared, view_ptr_assignment)
{
	auto p1 = shared<B>::allocate(10, 20).view();
	CHECK_EXPECT(B::nb, 1);
	CHECK(p1);

	shared<B>::view_ptr p2;
	CHECK(!p2);

	p2 = p1;
	CHECK(p2);
	CHECK_EXPECT(p1.get(), p2.get());
}

DEF_TEST(shared, ptr_assignment)
{
	auto p1 = shared<B>::allocate(10, 20);
	CHECK_EXPECT(B::nb, 1);
	CHECK(p1);

	shared<B>::ptr p2;
	CHECK(!p2);

	p2 = p1;
	CHECK(p2);
	CHECK_EXPECT(p1.get(), p2.get());
}

DEF_TEST(shared, view_ptr_move_operations)
{
	auto p1 = shared<B>::allocate(10, 20).view();
	CHECK_EXPECT(B::nb, 1);
	CHECK(p1);
	shared<B>::view_ptr p2(std::move(p1));
	CHECK(!p1);
	CHECK(p2);

	p1 = std::move(p2);
	CHECK(p1);
	CHECK(!p2);
}

DEF_TEST(shared, ptr_move_operations)
{
	auto p1 = shared<B>::allocate(10, 20);
	CHECK_EXPECT(B::nb, 1);
	CHECK(p1);
	shared<B>::ptr p2(std::move(p1));
	CHECK(!p1);
	CHECK(p2);

	p1 = std::move(p2);
	CHECK(p1);
	CHECK(!p2);
}

DEF_GROUP(shared_array)

DEF_TEST(shared_array, allocate)
{
	auto p = shared_array<B>::allocate(2, 10, 20);
	CHECK_EXPECT(B::nb, 2);

	auto x = p.get();
	CHECK_EXPECT(p[0].a, 10);
	CHECK_EXPECT(p[0].b, 20);
	CHECK_EXPECT(p[1].a, 10);
	CHECK_EXPECT(p[1].b, 20);
}

DEF_TEST(shared_array, write_shared)
{
	auto p = shared_array<B>::allocate(2, 10, 20);
	CHECK_EXPECT(B::nb, 2);

	CHECK_EXPECT(p[0].b, 20);
	CHECK_EXPECT(p[0].a, 10);
	CHECK_EXPECT(p[1].a, 10);
	CHECK_EXPECT(p[1].b, 20);

	auto p2 = p.view();
	auto p3 = p2.view();
	CHECK_EXPECT(B::nb, 2);
	auto pw = p3.write();
	CHECK_EXPECT(B::nb, 4);
	pw[0].a = 11;
	pw[1].b = 22;

	CHECK_EXPECT(pw[0].a, 11);
	CHECK_EXPECT(pw[0].b, 20);
	CHECK_EXPECT(pw[1].a, 10);
	CHECK_EXPECT(pw[1].b, 22);

	CHECK_EXPECT(p2[0].a, 11);
	CHECK_EXPECT(p2[0].b, 20);
	CHECK_EXPECT(p2[1].a, 10);
	CHECK_EXPECT(p2[1].b, 22);

	CHECK_EXPECT(p3[0].a, 11);
	CHECK_EXPECT(p3[0].b, 20);
	CHECK_EXPECT(p3[1].a, 10);
	CHECK_EXPECT(p3[1].b, 22);

	CHECK_EXPECT(p[0].a, 10);
	CHECK_EXPECT(p[0].b, 20);
	CHECK_EXPECT(p[1].a, 10);
	CHECK_EXPECT(p[1].b, 20);

}

DEF_TEST(shared_array, write_shared_2)
{
	auto p = shared_array<B>::allocate(2, 10, 20);
	p.write()[1].a = 30;
	p.write()[1].b = 40;
	CHECK_EXPECT(B::nb, 2);

	CHECK_EXPECT(p[0].a, 10);
	CHECK_EXPECT(p[0].b, 20);
	CHECK_EXPECT(p[1].a, 30);
	CHECK_EXPECT(p[1].b, 40);

	auto p2 = p.view();
	auto p3 = p2.view();
	CHECK_EXPECT(B::nb, 2);
	auto pw = p3.write();
	CHECK_EXPECT(B::nb, 4);

	CHECK_EXPECT(p2[0].a, 10);
	CHECK_EXPECT(p2[0].b, 20);
	CHECK_EXPECT(p2[1].a, 30);
	CHECK_EXPECT(p2[1].b, 40);

	CHECK_EXPECT(pw[0].a, 10);
	CHECK_EXPECT(pw[0].b, 20);
	CHECK_EXPECT(pw[1].a, 30);
	CHECK_EXPECT(pw[1].b, 40);

	pw[0].a = 1;
	pw[0].b = 2;
	pw[1].a = 3;
	pw[1].b = 4;

	CHECK_EXPECT(p2[0].a, 1);
	CHECK_EXPECT(p2[0].b, 2);
	CHECK_EXPECT(p2[1].a, 3);
	CHECK_EXPECT(p2[1].b, 4);

	CHECK_EXPECT(p3[0].a, 1);
	CHECK_EXPECT(p3[0].b, 2);
	CHECK_EXPECT(p3[1].a, 3);
	CHECK_EXPECT(p3[1].b, 4);

	CHECK_EXPECT(pw[0].a, 1);
	CHECK_EXPECT(pw[0].b, 2);
	CHECK_EXPECT(pw[1].a, 3);
	CHECK_EXPECT(pw[1].b, 4);

	CHECK_EXPECT(p[0].a, 10);
	CHECK_EXPECT(p[0].b, 20);
	CHECK_EXPECT(p[1].a, 30);
	CHECK_EXPECT(p[1].b, 40);


}
