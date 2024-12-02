#include <PulsarCore/GC/Pointer.hpp>
#include <gtest/gtest.h>

using namespace Pulsar;

class TestClass {
public:
    TestClass() = default;
    explicit TestClass(int i) : m_I(i) {
    }

    [[nodiscard]] int get() const {
        return m_I;
    }

private:
    int m_I;
};

TEST(Pointer, Scoped) {
    GC::Scoped<int> scoped(new int(5));
    EXPECT_EQ(*scoped, 5);

    GC::Scoped<TestClass> testClassScoped(new TestClass(5));
    EXPECT_EQ(testClassScoped->get(), 5);

    scoped.reset();
    EXPECT_EQ(scoped.get(), nullptr);
    testClassScoped.reset();
    EXPECT_EQ(testClassScoped.get(), nullptr);
}
