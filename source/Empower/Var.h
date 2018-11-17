/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2018
 *
 *  @file  Var.h
 *  @brief A collection of information about a single, instantiated variable in Empower
 */

#ifndef EMP_EMPOWER_VAR_H
#define EMP_EMPOWER_VAR_H

#include "../base/assert.h"
#include "../base/Ptr.h"

#include "MemoryImage.h"
#include "Type.h"

namespace emp {

  class Var {
  private:
    const Type & type;       ///< What type is this variable?
    MemoryImage & memory;    ///< Which memory image is variable using (by default)
    size_t mem_pos;          ///< Where is this variable in a memory image?

  public:
    Var(const Type & _type, MemoryImage & _mem, size_t _pos)
      : type(_type), memory(_mem), mem_pos(_pos) { ; }
    Var(const Var &) = default;

    /// Restore this variable to its orignal type for direct use.
    template <typename T>
    T & Restore() {
      // Make sure function is restoring the correct type.
      emp_assert( type.GetID() == GetTypeID<T>(), "Trying to restore Var to incorrect type." );

      // Convert this memory to a reference that can be returned.
      return memory.GetRef<T>(mem_pos);
    }
  };


}

#endif