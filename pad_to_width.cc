/*
 * Copyright (c) 1999-2010 Stephen Williams (steve@icarus.com)
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

# include  "netlist.h"
# include  "netmisc.h"

/*
 * This function transforms an expression by padding the high bits
 * with V0 until the expression has the desired width. This may mean
 * not transforming the expression at all, if it is already wide
 * enough.
 */
NetExpr*pad_to_width(NetExpr*expr, unsigned wid)
{
      if (wid <= expr->expr_width())
	    return expr;

	/* If the expression is a const, then replace it with a wider
	   const. This is a more efficient result. */
      if (NetEConst*tmp = dynamic_cast<NetEConst*>(expr)) {
	    verinum eval = tmp->value();
	    bool signed_flag = eval.has_sign();

	    verinum::V pad = verinum::V0;
	    if (signed_flag)
		  pad = eval.get(eval.len()-1);
	    verinum oval (pad, wid, eval.has_len());

	    for (unsigned idx = 0 ;  idx < eval.len() ;  idx += 1)
		  oval.set(idx, eval.get(idx));

	    oval.has_sign(signed_flag);
	    tmp = new NetEConst(oval);
	    delete expr;
	    return tmp;
      }

	/* Do it the hard way, with a concatenation. */
      verinum pad(verinum::V0, wid - expr->expr_width());
      NetEConst*co = new NetEConst(pad);
      co->set_line(*expr);
      NetEConcat*cc = new NetEConcat(2);
      cc->set_line(*expr);
      cc->set(0, co);
      cc->set(1, expr);
      cc->set_width(wid);
      expr = cc;

      return expr;
}

NetNet*pad_to_width(Design*des, NetNet*net, unsigned wid)
{
      NetScope*scope = net->scope();
      const string path = scope->name();
      assert(scope);

      if (net->pin_count() >= wid)
	    return net;

      verinum pad(verinum::V0, wid - net->pin_count());
      NetConst*con = new NetConst(scope, scope->local_symbol(), pad);
      des->add_node(con);

      NetNet*tmp = new NetNet(scope, scope->local_symbol(),
			      NetNet::WIRE, wid);
      tmp->local_flag(true);

      for (unsigned idx = 0 ;  idx < net->pin_count() ;  idx += 1)
	    connect(tmp->pin(idx), net->pin(idx));
      for (unsigned idx = net->pin_count() ;  idx < wid ;  idx += 1)
	    connect(tmp->pin(idx), con->pin(idx-net->pin_count()));

      return tmp;
}
