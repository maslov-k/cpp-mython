#include "statement.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>

using namespace std;

namespace ast
{

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace
{
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context)
{
    closure[var_] = rv_->Execute(closure, context);
    return closure.at(var_);
}

Assignment::Assignment(string var, unique_ptr<Statement> rv)
    : var_(move(var)), rv_(move(rv))
{
}

VariableValue::VariableValue(const string& var_name)
    : var_name_(var_name)
{
}

VariableValue::VariableValue(vector<string> dotted_ids)
{
    if (auto size = dotted_ids.size(); size > 0)
    {
        var_name_ = move(dotted_ids.at(0));
        dotted_ids_.resize(size - 1);
        move(next(dotted_ids.begin()), dotted_ids.end(), dotted_ids_.begin());
    }
}

ObjectHolder VariableValue::Execute(Closure& closure, Context& context)
{
    if (closure.count(var_name_))
    {
        auto result = closure.at(var_name_);
        if (dotted_ids_.size() > 0)
        {
            if (auto obj = result.TryAs<runtime::ClassInstance>())
            {
                return VariableValue(dotted_ids_).Execute(obj->Fields(), context);
            }
            else
            {
                throw runtime_error("Variable " + var_name_+ " is not a class"s);
            }
        }
        return result;
    }
    else
    {
        throw runtime_error("Variable "s + var_name_ + " not found"s);
    }
}

unique_ptr<Print> Print::Variable(const string& name)
{
    return make_unique<Print>(make_unique<VariableValue>(name));
}

Print::Print(unique_ptr<Statement> argument)
{
    args_.push_back(move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args)
    : args_(move(args))
{
}

ObjectHolder Print::Execute(Closure& closure, Context& context)
{
    bool first = true;
    auto &os = context.GetOutputStream();
    for (auto &arg : args_) {
        if (!first) {
            os << " "s;
        }
        auto obj = arg->Execute(closure, context);
        if (obj) {
            obj->Print(os, context);
        } else {
            os << "None"s;
        }
        first = false;
    }
    os << "\n"s;
    return {};
}

MethodCall::MethodCall(unique_ptr<Statement> object, string method,
                       vector<unique_ptr<Statement>> args)
    : object_(move(object)), method_(move(method)), args_(move(args))
{
}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context)
{
    runtime::ClassInstance* class_instance = object_->Execute(closure, context).TryAs<runtime::ClassInstance>();
    if (class_instance)
    {
        vector<runtime::ObjectHolder> actual_args;
        for (auto& arg : args_)
        {
            actual_args.push_back(arg->Execute(closure, context));
        }
        return class_instance->Call(method_, actual_args, context);
    }
    else
    {
        throw runtime_error("Object is not a class"s);
    }
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context)
{
    ObjectHolder object_holder = argument_->Execute(closure, context);
    if (object_holder)
    {
        ostringstream os;
        object_holder->Print(os, context);
        return ObjectHolder::Own(runtime::String(os.str()));
    }
    else
    {
        return ObjectHolder::Own(runtime::String("None"s));
    }
}

ObjectHolder Add::Execute(Closure& closure, Context& context)
{
    ObjectHolder lhs_holder = lhs_->Execute(closure, context);
    ObjectHolder rhs_holder = rhs_->Execute(closure, context);

    runtime::Number* lhs_number = lhs_holder.TryAs<runtime::Number>();
    runtime::Number* rhs_number = rhs_holder.TryAs<runtime::Number>();
    if (lhs_number && rhs_number)
    {
        return ObjectHolder::Own(runtime::Number(lhs_number->GetValue() +
                                                 rhs_number->GetValue()));
    }
    runtime::String* lhs_string = lhs_holder.TryAs<runtime::String>();
    runtime::String* rhs_string = rhs_holder.TryAs<runtime::String>();
    if (lhs_string && rhs_string)
    {
        return ObjectHolder::Own(runtime::String(lhs_string->GetValue() +
                                                 rhs_string->GetValue()));
    }
    runtime::ClassInstance* class_instance = lhs_holder.TryAs<runtime::ClassInstance>();
    if (class_instance)
    {
        return class_instance->Call(ADD_METHOD, {rhs_holder}, context);
    }
    throw runtime_error("Add error"s);
}

ObjectHolder Sub::Execute(Closure& closure, Context& context)
{
    ObjectHolder lhs_holder = lhs_->Execute(closure, context);
    ObjectHolder rhs_holder = rhs_->Execute(closure, context);

    runtime::Number* lhs_number = lhs_holder.TryAs<runtime::Number>();
    runtime::Number* rhs_number = rhs_holder.TryAs<runtime::Number>();
    if (lhs_number && rhs_number)
    {
        return ObjectHolder::Own(runtime::Number(lhs_number->GetValue() -
                                                 rhs_number->GetValue()));
    }
    throw runtime_error("Subtract error"s);
}

ObjectHolder Mult::Execute(Closure& closure, Context& context)
{
    ObjectHolder lhs_holder = lhs_->Execute(closure, context);
    ObjectHolder rhs_holder = rhs_->Execute(closure, context);

    runtime::Number* lhs_number = lhs_holder.TryAs<runtime::Number>();
    runtime::Number* rhs_number = rhs_holder.TryAs<runtime::Number>();
    if (lhs_number && rhs_number)
    {
        return ObjectHolder::Own(runtime::Number(lhs_number->GetValue() *
                                                 rhs_number->GetValue()));
    }
    throw runtime_error("Multiply error"s);
}

ObjectHolder Div::Execute(Closure& closure, Context& context)
{
    ObjectHolder lhs_holder = lhs_->Execute(closure, context);
    ObjectHolder rhs_holder = rhs_->Execute(closure, context);

    runtime::Number* lhs_number = lhs_holder.TryAs<runtime::Number>();
    runtime::Number* rhs_number = rhs_holder.TryAs<runtime::Number>();
    if (rhs_number && rhs_number->GetValue() == 0)
    {
        throw runtime_error("Divison by zero"s);
    }
    else if (lhs_number && rhs_number)
    {
        return ObjectHolder::Own(runtime::Number(lhs_number->GetValue() /
                                                 rhs_number->GetValue()));
    }
    throw runtime_error("Divison error"s);
}

ObjectHolder Compound::Execute(Closure& closure, Context& context)
{
    for (auto &arg : args_)
    {
        arg->Execute(closure, context);
    }
    return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure& closure, Context& context)
{
    throw statement_->Execute(closure, context);
}

ClassDefinition::ClassDefinition(ObjectHolder cls)
    : cls_(move(cls))
{
}

ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]] Context& context)
{
    closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
    return ObjectHolder::None();
}

FieldAssignment::FieldAssignment(VariableValue object, string field_name,
                                 unique_ptr<Statement> rv)
    : object_(move(object)), field_name_(move(field_name)), rv_(move(rv))
{
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context)
{
    runtime::ClassInstance* class_instance = object_.Execute(closure, context).TryAs<runtime::ClassInstance>();
    if (class_instance)
    {
        return class_instance->Fields()[field_name_] = rv_->Execute(closure, context);
    }
    else
    {
        throw runtime_error("Not a class"s);
    }
}

IfElse::IfElse(unique_ptr<Statement> condition, unique_ptr<Statement> if_body,
               unique_ptr<Statement> else_body)
    : condition_(move(condition)), if_body_(move(if_body)), else_body_(move(else_body))
{
}

ObjectHolder IfElse::Execute(Closure& closure, Context& context)
{
    if (runtime::IsTrue(condition_->Execute(closure, context)))
    {
        return if_body_->Execute(closure, context);
    }
    else if (else_body_)
    {
        return else_body_->Execute(closure, context);
    }
    return runtime::ObjectHolder::None();
}

ObjectHolder Or::Execute(Closure& closure, Context& context)
{
    if (runtime::IsTrue(lhs_->Execute(closure, context)))
    {
        return ObjectHolder::Own(runtime::Bool(true));
    }
    return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(rhs_->Execute(closure, context))));
}

ObjectHolder And::Execute(Closure& closure, Context& context)
{
    if (runtime::IsTrue(lhs_->Execute(closure, context)))
    {
        return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(rhs_->Execute(closure, context))));
    }
    return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder Not::Execute(Closure& closure, Context& context)
{
    bool result = !runtime::IsTrue(argument_->Execute(closure, context));
    return ObjectHolder::Own(runtime::Bool(result));
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(move(lhs), move(rhs))
{
    cmp_ = move(cmp);
}

ObjectHolder Comparison::Execute(Closure& closure, Context& context)
{
    bool result = cmp_(lhs_->Execute(closure, context), rhs_->Execute(closure, context), context);
    return runtime::ObjectHolder::Own(runtime::Bool(result));
}

NewInstance::NewInstance(const runtime::Class& class_, vector<unique_ptr<Statement>> args)
    : class_instance_(class_), args_(std::move(args))
{
}

NewInstance::NewInstance(const runtime::Class& class_)
    : NewInstance(class_, {})
{
}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context)
{
    if (class_instance_.HasMethod(INIT_METHOD, args_.size()))
    {
        std::vector<runtime::ObjectHolder> actual_args;
        for (auto &arg : args_)
        {
            actual_args.push_back(arg->Execute(closure, context));
        }
        class_instance_.Call(INIT_METHOD, actual_args, context);
    }
    return runtime::ObjectHolder::Share(class_instance_);
}

MethodBody::MethodBody(unique_ptr<Statement>&& body)
    : body_(std::move(body))
{
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context)
{
    try
    {
        body_->Execute(closure, context);
        return runtime::ObjectHolder::None();
    }
    catch (runtime::ObjectHolder& result)
    {
        return result;
    }
}

}  // namespace ast
