/*
 * Copyright (c) 1998-2010 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

# include "config.h"

# include  <iostream>

# include  "PExpr.h"
# include  "netlist.h"
# include  "compiler.h"

verinum* PExpr::eval_const(const Design*, const NetScope*) const
{
      return 0;
}

verinum* PEBinary::eval_const(const Design*des, const NetScope*scope) const
{
      verinum*l = left_->eval_const(des, scope);
      if (l == 0) return 0;
      verinum*r = right_->eval_const(des, scope);
      if (r == 0) {
	    delete l;
	    return 0;
      }
      verinum*res;

      switch (op_) {
	  case '+': {
		if (l->is_defined() && r->is_defined()) {
		      res = new verinum(*l + *r);
		} else {
		      res = new verinum(verinum::Vx, l->len());
		}
		break;
	  }
	  case '-': {
		if (l->is_defined() && r->is_defined()) {
		      res = new verinum(*l - *r);
		} else {
		      res = new verinum(verinum::Vx, l->len());
		}
		break;
	  }
	  case '*': {
		if (l->is_defined() && r->is_defined()) {
		      res = new verinum(*l * *r);
		} else {
		      res = new verinum(verinum::Vx, l->len());
		}
		break;
	  }
	  case '/': {
		if (l->is_defined() && r->is_defined()) {
		      long lv = l->as_long();
		      long rv = r->as_long();
		      res = new verinum(lv / rv, l->len());
		} else {
		      res = new verinum(verinum::Vx, l->len());
		}
		break;
	  }
	  case '%': {
		if (l->is_defined() && r->is_defined()) {
		      long lv = l->as_long();
		      long rv = r->as_long();
		      res = new verinum(lv % rv, l->len());
		} else {
		      res = new verinum(verinum::Vx, l->len());
		}
		break;
	  }
	  case '>': {
		if (l->is_defined() && r->is_defined()) {
		      long lv = l->as_long();
		      long rv = r->as_long();
		      res = new verinum(lv > rv, l->len());
		} else {
		      res = new verinum(verinum::Vx, l->len());
		}
		break;
	  }
	  case '<': {
		if (l->is_defined() && r->is_defined()) {
		      long lv = l->as_long();
		      long rv = r->as_long();
		      res = new verinum(lv < rv, l->len());
		} else {
		      res = new verinum(verinum::Vx, l->len());
		}
		break;
	  }
	  case 'l': { // left shift (<<)
		assert(r->is_defined());
		unsigned long rv = r->as_ulong();
		unsigned use_wid = l->len();
		if (! l->has_len())
		      use_wid += rv;
		res = new verinum(verinum::V0, use_wid);
		if (rv < res->len()) {
		      unsigned cnt = res->len() - rv;
		      for (unsigned idx = 0 ;  idx < cnt ;  idx += 1)
			    res->set(idx+rv, l->get(idx));
		}
		break;
	  }
	  case 'r': { // right shift (>>)
		assert(r->is_defined());
		unsigned long rv = r->as_ulong();
		res = new verinum(verinum::V0, l->len());
		if (rv < res->len()) {
		      unsigned cnt = res->len() - rv;
		      for (unsigned idx = 0 ;  idx < cnt ;  idx += 1)
			    res->set(idx, l->get(idx+rv));
		}
		break;
	  }

	  default:
	    delete l;
	    delete r;
	    return 0;
      }

      delete l;
      delete r;
      return res;
}

verinum* PEConcat::eval_const(const Design*des, const NetScope*scope) const
{
      verinum*accum = parms_[0]->eval_const(des, scope);
      if (accum == 0)
	    return 0;

      for (unsigned idx = 1 ;  idx < parms_.count() ;  idx += 1) {
	    verinum*tmp = parms_[idx]->eval_const(des, scope);
	      /* Oops, found an argument that is not constant. Give up. */
	    if (tmp == 0) {
		  delete accum;
		  return 0;
	    }
	    assert(tmp);

	    *accum = concat(*accum, *tmp);
	    delete tmp;
      }

      return accum;
}

/*
 * Evaluate an identifier as a constant expression. This is only
 * possible if the identifier is that of a parameter.
 */
verinum* PEIdent::eval_const(const Design*des, const NetScope*scope) const
{
      assert(scope);
      const NetExpr*expr = des->find_parameter(scope, path_);

      if (expr == 0)
	    return 0;

      const NetEConst*eval = dynamic_cast<const NetEConst*>(expr);
      if (eval == 0) {
	    cerr << get_line() << ": internal error: Unable to evaluate "
		 << "constant expression (parameter=" << path_
		 << "): " << *expr << endl;
	    return 0;
      }

      assert(eval);

      if (msb_ || lsb_)
	    return 0;


      return new verinum(eval->value());
}

verinum* PEFNumber::eval_const(const Design*, const NetScope*) const
{
      long val = value_->as_long();
      return new verinum(val);
}

verinum* PENumber::eval_const(const Design*, const NetScope*) const
{
      return new verinum(value());
}

verinum* PEString::eval_const(const Design*, const NetScope*) const
{
      return new verinum(string(text_));
}

verinum* PETernary::eval_const(const Design*des, const NetScope*scope) const
{
      verinum*test = expr_->eval_const(des, scope);
      if (test == 0)
	    return 0;

      verinum::V bit = test->get(0);
      delete test;
      switch (bit) {
	  case verinum::V0:
	    return fal_->eval_const(des, scope);
	  case verinum::V1:
	    return tru_->eval_const(des, scope);
	  default:
	    return 0;
	      // XXXX It is possible to handle this case if both fal_
	      // and tru_ are constant. Someday...
      }
}

verinum* PEUnary::eval_const(const Design*des, const NetScope*scope) const
{
      verinum*val = expr_->eval_const(des, scope);
      if (val == 0)
	    return 0;

      switch (op_) {
	  case '+':
	    return val;

	  case '-': {
		  /* We need to expand the value a bit if we are
		     taking the 2's complement so that we are
		     guaranteed to not overflow. */
		verinum tmp (0UL, val->len()+1);
		for (unsigned idx = 0 ;  idx < val->len() ;  idx += 1)
		      tmp.set(idx, val->get(idx));

		*val = v_not(tmp) + verinum(verinum::V1, 1);
		val->has_sign(true);
		return val;
	  }

      }
	    delete val;
      return 0;
}
