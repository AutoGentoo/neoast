//
// Created by tumbar on 6/21/21.
//

#include <reflex/pattern.h>
#include <vector>

class MyClass
{
    std::string processed;
    reflex::Pattern p;

public:
    explicit MyClass(const std::string& str) : processed(str + "CHANGED")
    {
        p.assign(processed);
    }

    const reflex::Pattern& get_pattern() const { return p; }
};

int main(void)
{
    std::vector<MyClass> m;
    m.emplace_back("ABC[123]");
    return 0;
}