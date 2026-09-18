#pragma once

#include "Object.h"

template<typename TObject>
class TObjectIterator
{
public:
    TObjectIterator()
    {
        AdvanceToNextValidObject();
    }

    explicit operator bool() const
    {
        return CurrentIndex < UObject::GetGObjectArray().Max();
    }

    TObjectIterator& operator++()
    {
        ++CurrentIndex;
        AdvanceToNextValidObject();
        return *this;
    }

    TObject* operator*() const
    {
        return static_cast<TObject*>(UObject::GetGObjectArray()[CurrentIndex]);
    }

private:
    void AdvanceToNextValidObject()
    {
        TSparseArray<UObject*>& Objects = UObject::GetGObjectArray();

        while (CurrentIndex < Objects.Max())
        {
            if (Objects.IsValidIndex(CurrentIndex))
            {
                UObject* Object = Objects[CurrentIndex];
                if (Object && Object->IsA<TObject>())
                {
                    return;
                }
            }
            ++CurrentIndex;
        }
    }

private:
    int32 CurrentIndex = 0;
};