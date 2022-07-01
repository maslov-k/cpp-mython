#include "runtime.h"

#include <algorithm>
#include <cassert>
#include <optional>
#include <sstream>

using namespace std;

template <typename Comp>
bool Compare(const runtime::ObjectHolder& lhs, const runtime::ObjectHolder& rhs, Comp comp)
{
    runtime::Bool* bool_lhs = lhs.TryAs<runtime::Bool>();
    runtime::Bool* bool_rhs = rhs.TryAs<runtime::Bool>();
    if (bool_lhs && bool_rhs)
    {
        return comp(bool_lhs->GetValue(), bool_rhs->GetValue());
    }

    runtime::String* string_lhs = lhs.TryAs<runtime::String>();
    runtime::String* string_rhs = rhs.TryAs<runtime::String>();
    if (string_lhs && string_rhs)
    {
        return comp(string_lhs->GetValue(), string_rhs->GetValue());
    }

    runtime::Number* number_lhs = lhs.TryAs<runtime::Number>();
    runtime::Number* number_rhs = rhs.TryAs<runtime::Number>();
    if (number_lhs && number_rhs)
    {
        return comp(number_lhs->GetValue(), number_rhs->GetValue());
    }
    throw std::runtime_error("Wrong types to compare"s);
}

namespace runtime
{

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data))
{
}

void ObjectHolder::AssertIsValid() const
{
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object)
{
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None()
{
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const
{
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const
{
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const
{
    return data_.get();
}

ObjectHolder::operator bool() const
{
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder& object)
{
    if (Number* number_obj = object.TryAs<Number>())
    {
        return number_obj->GetValue() != 0;
    }
    else if (Bool* number_obj = object.TryAs<Bool>())
    {
        return number_obj->GetValue() == true;
    }
    else if (String* number_obj = object.TryAs<String>())
    {
        return !number_obj->GetValue().empty();
    }
    return false;
}

void ClassInstance::Print(std::ostream& os, Context& context)
{
    if (HasMethod("__str__"s, 0))
    {
        Call("__str__"s, {}, context)->Print(os, context);
    }
    else
    {
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const
{
    if (const Method* mtd = cls_.GetMethod(method))
    {
        return mtd->formal_params.size() == argument_count;
    }
    return false;
}

Closure& ClassInstance::Fields()
{
    return closure_;
}

const Closure& ClassInstance::Fields() const
{
    return closure_;
}

ClassInstance::ClassInstance(const Class& cls)
    : cls_(cls)
{
}

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                 Context& context)
{
    if (!HasMethod(method, actual_args.size()))
    {
        throw std::runtime_error("Class "s + cls_.GetName() + " don't have method "s + method +
                                 " with "s + std::to_string(actual_args.size()) +
                                 " argemets."s);
    }

    const Method* mtd = cls_.GetMethod(method);
    Closure closure;
    closure["self"s] = ObjectHolder::Share(*this);

    size_t index = 0;
    for (const string& param : mtd->formal_params)
    {
        closure[param] = actual_args.at(index++);
    }

    return mtd->body->Execute(closure, context);
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
    : name_(move(name)), methods_(move(methods)), parent_(parent)
{
}

const Method* Class::GetMethod(const std::string& name) const
{
    auto method = find_if(methods_.begin(), methods_.end(), [&name](const Method& mtd)
        {
            return mtd.name == name;
        });
    if (method != methods_.end())
    {
        return &*method;
    }
    else if (parent_)
    {
        return parent_->GetMethod(name);
    }
    return nullptr;
}

const std::string& Class::GetName() const
{
    return name_;
}

void Class::Print(ostream& os, [[maybe_unused]] Context& context)
{
    os << "Class "s << name_;
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context)
{
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
{
    ClassInstance* cls_inst = lhs.TryAs<ClassInstance>();
    if (cls_inst)
    {
        return IsTrue(cls_inst->Call("__eq__"s, {rhs}, context));
    }
    else if (!lhs && !rhs)
    {
        return true;
    }
    return Compare(lhs, rhs, equal_to());
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
{
    ClassInstance* cls_inst = lhs.TryAs<ClassInstance>();
    if (cls_inst)
    {
        return IsTrue(cls_inst->Call("__lt__"s, {rhs}, context));
    }
    return Compare(lhs, rhs, less());
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
{
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
{
    return !(Less(lhs, rhs, context) || Equal(lhs, rhs, context));
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
{
    return !Greater(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
{
    return !Less(lhs, rhs, context);
}

}  // namespace runtime
