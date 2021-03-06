/*
 * Copyright (c) 1998-2000 Stephen Williams (steve@icarus.com)
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
#ifdef HAVE_CVS_IDENT
#ident "$Id: t-xnf.cc,v 1.52 2004/02/20 18:53:36 steve Exp $"
#endif

# include "config.h"

# include  <iostream>

/* XNF BACKEND
 * This target supports generating Xilinx Netlist Format netlists for
 * use by Xilinx tools, and other tools that accepts Xilinx designs.
 *
 * The code generator automatically detects ports to top level modules
 * and generates SIG records that make the XNF usable as a schematic.
 *
 * FLAGS
 * The XNF backend uses the following flags from the command line to
 * affect the generated file:
 *
 *   part=<foo>
 *	Specify the part type. The part string is written into the
 *	PART record. Valid types are defined by Xilinx or the
 *	receiving tools.
 *
 *   ncf=<path>
 *      Specify the path to a NCF file. This is an OUTPUT file into
 *      which the code generator will write netlist constraints that
 *      relate to pin assignments, CLB placement, etc. If this flag is
 *      not given, no NCF file will be written.
 *
 * WIRE ATTRIBUTES
 *
 *   PAD = <io><n>
 *      Tell the XNF generator that this wire goes to a PAD. The <io>
 *      is a single character that tells the direction, and <n> is the
 *      pin number. For example, "o31" is output on pin 31. The PAD
 *      attribute is not practically connected to a vector, as all the
 *      bits would go to the same pad.
 *
 * NODE ATTRIBUTES
 *
 *   XNF-LCA = <lname>:<pin>,<pin>...
 *      Specify the LCA library part type for the gate. The lname
 *      is the name of the symbol to use (i.e. DFF) and the comma
 *      separated list is the names of the pins, in the order they
 *      appear in the Verilog source. If the name is prefixed with a
 *      tilde (~) then the pin is inverted, and the proper "INV" token
 *      will be added to the PIN record.
 *
 *      This attribute can override even the typical generation of
 *      gates that one might naturally expect of the code generator,
 *      but may be used by the optimizers for placing parts.
 *
 *      An example is "XNF-LCA=OBUF:O,~I". This attribute means that
 *      the object is an OBUF. Pin 0 is called "O", and pin 1 is
 *      called "I". In addition, pin 1 is inverted.
 */

# include  "netlist.h"
# include  "target.h"
# include  <fstream>
# include  <sstream>

verinum::V link_get_ival(const Link&lnk)
{
      const Nexus*nex = lnk.nexus();
      for (const Link*cur = nex->first_nlink()
		 ;  cur ;  cur = cur->next_nlink()) {
	    if (cur == &lnk)
		  continue;

	    if (dynamic_cast<const NetNet*>(cur->get_obj()))
		  return cur->nexus()->get_init();

      }

      return verinum::Vx;
}

class target_xnf  : public target_t {

    public:
      bool start_design(const Design*);
      int  end_design(const Design*);
      void memory(const NetMemory*);
      void signal(const NetNet*);

      void lpm_add_sub(const NetAddSub*);
      void lpm_compare(const NetCompare*);
      void lpm_compare_eq_(ostream&os, const NetCompare*);
      void lpm_compare_ge_(ostream&os, const NetCompare*);
      void lpm_compare_le_(ostream&os, const NetCompare*);
      void lpm_ff(const NetFF*);
      void lpm_mux(const NetMux*);
      void lpm_ram_dq(const NetRamDq*);

      bool net_const(const NetConst*);
      void logic(const NetLogic*);
      bool bufz(const NetBUFZ*);
      void udp(const NetUDP*);

    private:
      static string mangle(const string&);
      static string mangle(perm_string);
      static string choose_sig_name(const Link*lnk);
      static void draw_pin(ostream&os, const string&name,
			   const Link&lnk);
      static void draw_sym_with_lcaname(ostream&os, string lca,
					const NetNode*net);
      static void draw_xor(ostream&os, const NetAddSub*, unsigned idx);
      enum adder_type {FORCE0, LOWER, DOUBLE, LOWER_W_CO, EXAMINE_CI };
      static void draw_carry(ostream&os, const NetAddSub*, unsigned idx,
			     enum adder_type);

      ofstream out_;
      ofstream ncf_;
};

/*
 * This function takes a signal name and mangles it into an equivalent
 * name that is suitable to the XNF format.
 */
string target_xnf::mangle(const string&name)
{
      string result;
      for (unsigned idx = 0 ;  idx < name.length() ;  idx += 1)
	    switch (name[idx]) {
		case '.':
		  result = result + "/";
		  break;
		default:
		  result = result + name[idx];
		  break;
	    }

      return result;
}

string target_xnf::mangle(perm_string name)
{
      return mangle(string(name));
}

/*
 * This method takes a signal and pin number as a nexus. Scan the
 * nexus to decide which name to use if there are lots of attached
 * signals.
 */
string target_xnf::choose_sig_name(const Link*lnk)
{
      return mangle( string(lnk->nexus()->name()) );
}

void target_xnf::draw_pin(ostream&os, const string&name,
			  const Link&lnk)
{
      bool inv = false;
      string use_name = name;
      if (use_name[0] == '~') {
	    inv = true;
	    use_name = use_name.substr(1);
      }

      char type=0;
      switch (lnk.get_dir()) {
	  case Link::INPUT:
	  case Link::PASSIVE:
	    type = 'I';
	    break;
	  case Link::OUTPUT:
	    type = 'O';
	    break;
      }
      assert(type);

      os << "    PIN, " << use_name << ", " << type << ", " <<
	    choose_sig_name(&lnk);
      if (inv) os << ",,INV";
      os << endl;
}

static string scrape_pin_name(string&list)
{
      unsigned idx = list.find(',');
      string name = list.substr(0, idx);
      list = list.substr(idx+1);
      return name;
}

/*
 * This method draws an LCA item based on the XNF-LCA attribute
 * given. The LCA attribute gives enough information to completely
 * draw the node in XNF, which is pretty handy at this point.
 */
void target_xnf::draw_sym_with_lcaname(ostream&os, string lca,
				       const NetNode*net)
{
      unsigned idx = lca.find(':');
      string lcaname = lca.substr(0, idx);
      lca = lca.substr(idx+1);

      os << "SYM, " << mangle(net->name()) << ", " << lcaname
	 << ", LIBVER=2.0.0" << endl;

      for (idx = 0 ;  idx < net->pin_count() ;  idx += 1) {
	    string usename = scrape_pin_name(lca);
	    if (usename == "") continue;
	    draw_pin(os, usename, net->pin(idx));
      }

      os << "END" << endl;
}

bool target_xnf::start_design(const Design*des)
{
      out_.open(des->get_flag("-o"), ios::out | ios::trunc);

      string ncfpath = des->get_flag("ncf");
      if (ncfpath != "")
	    ncf_.open(ncfpath.c_str());

      out_ << "LCANET,6" << endl;
      out_ << "PROG,verilog,$Name: s20040606 $,\"Icarus Verilog\"" << endl;
      ncf_ << "# Generated by Icarus Verilog $Name: s20040606 $" << endl;

      if (des->get_flag("part") != 0) {
	    out_ << "PART," << des->get_flag("part") << endl;
	    ncf_ << "CONFIG PART=" << des->get_flag("part") << ";" << endl;
      }

      return true;
}

int target_xnf::end_design(const Design*)
{
      out_ << "EOF" << endl;
      ncf_.close();
      return 0;
}

void scrape_pad_info(string str, char&dir, unsigned&num)
{
	// Get rid of leading white space
      while (str[0] == ' ')
	    str = str.substr(1);

	// Get the direction letter
      switch (str[0]) {
	  case 'b':
	  case 'B':
	    dir = 'B';
	    break;
	  case 'o':
	  case 'O':
	    dir = 'O';
	    break;
	  case 'i':
	  case 'I':
	    dir = 'I';
	    break;
	  case 't':
	  case 'T':
	    dir = 'T';
	    break;
	  default:
	    dir = '?';
	    break;
      }

	// Get the number part.
      str = str.substr(1);
      unsigned val = 0;
      while (str.size() && isdigit(str[0])) {
	    val = val * 10 + (str[0]-'0');
	    str = str.substr(1);
      }
      num = val;
}

/*
 * Memories are handled by the lpm_ram_dq method, so there is nothing
 * to do here.
 */
void target_xnf::memory(const NetMemory*)
{
}

/*
 * Look for signals that have attributes that are pertinent to XNF
 * files. The most obvious are those that have the PAD attribute.
 *
 * Individual signals are easy, the pad description is a letter
 * followed by a decimal number that is the pin.
 *
 * The PAD attribute for a vector is a comma separated pin
 * descriptions, that enumerate the pins from most significant to
 * least significant.
 */
void target_xnf::signal(const NetNet*net)
{

	/* Look for signals that are ports to the root module. If they
	   are, the write a SIG record and generate a pin name so that
	   this module can be used as a macro. */

      if (const NetScope*scope = net->scope()) do {

	    if (scope->parent())
		  break;

	    if (net->port_type() == NetNet::NOT_A_PORT)
		  break;

	    string mname = mangle(net->name());
	    string pname = mname.substr(mname.find('/')+1, mname.length());

	    if (net->pin_count() == 1) {
		  out_ << "SIG, " << mangle(net->name()) << ", PIN="
		     << pname << endl;

	    } else for (unsigned idx = 0; idx < net->pin_count(); idx += 1) {
		  out_ << "SIG, " << mangle(net->name()) << "<" << idx
		     << ">, PIN=" << pname << idx << endl;
	    }

      } while (0);


	/* Now look to see if a PAD attribute is attached, and if so
	   write out PAD information to the XNF and the ncf files. */

      string pad = net->attribute(perm_string::literal("PAD")).as_string();
      if (pad == "")
	    return;

      if (net->pin_count() > 1) {
	    cerr << "Signal ``" << net->name() << "'' with PAD=" <<
		  pad << " is a vector." << endl;
	    return;
      }

      char dir;
      unsigned num;
      scrape_pad_info(pad, dir, num);
      out_ << "EXT, " << mangle(net->name()) << ", " << dir
	 << ", " << num << endl;

      ncf_ << "# Assignment to pin " << num << " (DIR=" << dir <<
	    ") by $attribute(" << net->name() << ", \"PAD\", \"" <<
	    pad << "\")" << endl;
      ncf_ << "NET " << mangle(net->name()) << " LOC=P" << num << ";"
	   << endl;
}

void target_xnf::draw_xor(ostream &os, const NetAddSub*gate, unsigned idx)
{
      string name = mangle(gate->name());
      string name_add = name;
      string name_cout = name + "/COUT";

	      // We only need to pick up the
	      // carry if we are not the 0 bit. (We know it is 0).
      os << "SYM, " << name_add << "<" << (idx+0) << ">, XOR, "
	  "LIBVER=2.0.0" << endl;
      draw_pin(os, "O",  gate->pin_Result(idx));
      draw_pin(os, "I0", gate->pin_DataA(idx));
      draw_pin(os, "I1", gate->pin_DataB(idx));
      if (idx > 0) {
	  os << "    PIN, I2, I, " << name_cout << "<" <<
		idx << ">" << endl;
      }
      os << "END" << endl;
}

void target_xnf::draw_carry(ostream &os, const NetAddSub*gate, unsigned idx,
      enum adder_type type)
{
      string name = mangle(gate->name());

      string name_cy4 = name + "/CY";
      string name_cym = name + "/CM";
      string name_cout = name + "/COUT";

      os << "SYM, " << name_cy4 << "<" << idx << ">, CY4, "
	    "LIBVER=2.0.0" << endl;

      // Less significant bit addends, if any
      if ( type == LOWER || type == DOUBLE || type == LOWER_W_CO ) {
	    draw_pin(os, "A0", gate->pin_DataA(idx));
	    draw_pin(os, "B0", gate->pin_DataB(idx));
      }

      // More significant bit addends, if any
      if ( type == DOUBLE ) {
	    draw_pin(os, "A1", gate->pin_DataA(idx+1));
	    draw_pin(os, "B1", gate->pin_DataB(idx+1));
      }

      // All but FORCE0 cells have carry input
      if ( type != FORCE0 ) {
	  os  << "    PIN, CIN, I, " << name_cout << "<" << idx << ">" << endl;
      }

      // Connect the Cout0 to a signal so that I can connect
      // it to the adder.
      switch (type) {
	  case LOWER:
	  case DOUBLE:
	    os << "    PIN, COUT0, O, " << name_cout << "<" << (idx+1) <<
		  ">" << endl;
	    break;
	  case EXAMINE_CI:
	  case LOWER_W_CO:
	    draw_pin(os, "COUT0", gate->pin_Cout());
	    break;

	  default:
	    assert(0);
      }

      // Connect the Cout, this will connect to the next Cin
      if ( type == FORCE0 || type == DOUBLE ) {
	    unsigned int to = (type==FORCE0)?(0):(idx+2);
	    os << "    PIN, COUT, O, " << name_cout << "<" << to <<
		  ">" << endl;
      }

      // These are the mode inputs from the CY_xx pseudo-device
      for (unsigned cn = 0 ;  cn < 8 ;  cn += 1) {
	    os << "    PIN, C" << cn << ", I, " << name << "/C"
	       << cn << "<" << (idx) << ">" << endl;
      }
      os << "END" << endl;

      // On to the CY_xx pseudo-device itself
      os << "SYM, " << name_cym << "<" << (idx) << ">, ";
      switch (type) {
	  case FORCE0:
	    os << "CY4_37, CYMODE=FORCE-0" << endl;
	    break;
	  case LOWER:
	    os << "CY4_01, CYMODE=ADD-F-CI" << endl;
	    break;
	  case LOWER_W_CO:
	    os << "CY4_01, CYMODE=ADD-F-CI" << endl;
	    break;
	  case DOUBLE:
	    os << "CY4_02, CYMODE=ADD-FG-CI" << endl;
	    break;
	  case EXAMINE_CI:
	    os << "CY4_42, CYMODE=EXAMINE-CI" << endl;
	    break;
      }
      for (unsigned cn = 0 ;  cn < 8 ;  cn += 1) {
	    os << "    PIN, C" << cn << ", O, " << name << "/C"
	       << cn << "<" << (idx) << ">" << endl;
      }
      os << "END" << endl;
}

/*
 * This function makes an adder out of carry logic symbols. It makes
 * as many 2 bit adders as are possible, then the top bit is made into
 * a 1-bit adder (with carry in) in the F unit. The low carry is
 * initialized with the FORCE-0 configuration of a carry unit below
 * the 0 bit. This takes up the carry logic of the CLB below, but not
 * the G function.
 *
 * References:
 *    XNF 6.1 Specification
 *    Application note XAPP013
 *    Xilinx Libraries Guide, Chapter 12
 */
void target_xnf::lpm_add_sub(const NetAddSub*gate)
{
      unsigned width = gate->width();

	/* Make the force-0 carry mode object to initialize the bottom
	   bits of the carry chain. Label this with the width instead
	   of the bit position so that symbols don't clash. */

      draw_carry(out_, gate, width+1, FORCE0);


	/* Now make the 2 bit adders that chain from the cin
	   initializer and up. Save the tail bits for later. */
      for (unsigned idx = 0 ;  idx < width-2 ;  idx += 2)
	    draw_carry(out_, gate, idx, DOUBLE);

	/* Always have one or two tail bits. The situation gets a
	   little tricky if we want the carry output, so handle that
	   here.

	   If the carry-out is connected, and there are an even number
	   of data bits, we need to see the cout from the CLB. This is
	   done by configuring the top CLB CY device as ADD-FG-CI (to
	   activate cout) and create an extra CLB CY device on top of
	   the carry chain configured EXAMINE-CI to put the carry into
	   the G function block.

	   IF the carry-out is connected and there are an odd number
	   of data bits, then the top CLB can be configured to carry
	   the top bit in the F unit and deliver the carry out through
	   the G unit.

	   If the carry-out is not connected, then configure this top
	   CLB as ADD-F-CI. The draw_xor for the top bit will include
	   the F carry if needed. */

      if (gate->pin_Cout().is_linked()) {
	    if (width%2 == 0) {
		  draw_carry(out_, gate, width-2, DOUBLE);
		  draw_carry(out_, gate, width, EXAMINE_CI);
	    } else {
		  draw_carry(out_, gate, width-1, LOWER_W_CO);
	    }

      } else {
	    if (width%2 == 0)
		  draw_carry(out_, gate, width-2, LOWER);
	    else
		  draw_carry(out_, gate, width-1, LOWER);
      }

	/* Now draw all the single bit (plus carry in) adders from XOR
	   gates. This puts the F and G units to use. */
      for (unsigned idx = 0 ;  idx < width ;  idx += 1)
	    draw_xor(out_, gate, idx);

}

/*
 * In XNF, comparators are done differently depending on the type of
 * comparator being implemented. So, here we dispatch to the correct
 * code generator.
 */
void target_xnf::lpm_compare(const NetCompare*dev)
{
      if (dev->pin_AEB().is_linked() || dev->pin_ANEB().is_linked()) {
	    lpm_compare_eq_(out_, dev);
	    return;
      }

      if (dev->pin_AGEB().is_linked()) {
	    lpm_compare_ge_(out_, dev);
	    return;
      }

      if (dev->pin_ALEB().is_linked()) {
	    lpm_compare_le_(out_, dev);
	    return;
      }

      assert(0);
}

/*
 * To compare that vectors are equal (identity comparator) generate
 * XNOR gates to compare each pair of bits, then generate an AND gate
 * to combine the bitwise results. This is pretty much the best way to
 * do an identity compare in Xilinx CLBs.
 */
void target_xnf::lpm_compare_eq_(ostream&os, const NetCompare*dev)
{
      string mname = mangle(dev->name());

	/* Draw XNOR gates for each bit pair. These gates to the
	   bitwise comparison. */
      for (unsigned idx = 0 ;  idx < dev->width() ;  idx += 1) {
	    os << "SYM, " << mname << "/cmp<" << idx << ">, "
	       << "XNOR, LIBVER=2.0.0" << endl;
	    os << "    PIN, O, O, " << mname << "/bit<" << idx << ">"
	       << endl;
	    draw_pin(os, "I0", dev->pin_DataA(idx));
	    draw_pin(os, "I1", dev->pin_DataB(idx));
	    os << "END" << endl;
      }

	/* Now draw an AND gate to combine all the bitwise
	   comparisons. If there are more the 5 bits of data, then we
	   are going to have generate a nested AND to combine the
	   results. */

      if (dev->width() > 5) {
	    for (unsigned idx = 0 ;  idx < dev->width() ;  idx += 5) {

		  if ((idx+1) >= dev->width()) break;
		  os << "SYM, " << mname << "/nest<" << idx
		     << ">, AND, LIBVER=2.0.0" << endl;

		  os << "    PIN, O, O, " << mname << "/nbit<" << idx
		     << ">" << endl;

		  os << "    PIN, I0, I, " << mname << "/bit<" << idx+0
		     << ">" << endl;
		  os << "    PIN, I1, I, " << mname << "/bit<" << idx+1
		     << ">" << endl;
		  if ((idx+2) >= dev->width()) goto gate_out;
		  os << "    PIN, I2, I, " << mname << "/bit<" << idx+2
		     << ">" << endl;
		  if ((idx+3) >= dev->width()) goto gate_out;
		  os << "    PIN, I3, I, " << mname << "/bit<" << idx+3
		     << ">" << endl;
		  if ((idx+4) >= dev->width()) goto gate_out;
		  os << "    PIN, I4, I, " << mname << "/bit<" << idx+4
		     << ">" << endl;
	    gate_out:
		  os << "END" << endl;
	    }

	      /* Draw an AND gate if this is an EQ result, or a NAND
		 gate of this is a NEQ result. */

	    if (dev->pin_AEB().is_linked()) {
		  assert( ! dev->pin_ANEB().is_linked());
		  os << "SYM, " << mname << ", AND, LIBVER=2.0.0" << endl;
		  draw_pin(os, "O", dev->pin_AEB());

	    } else {
		  assert( dev->pin_ANEB().is_linked());
		  os << "SYM, " << mname << ", NAND, LIBVER=2.0.0" << endl;
		  draw_pin(os, "O", dev->pin_ANEB());
	    }

	    for (unsigned idx = 0 ;  idx < dev->width() ;  idx += 5) {
		  if ((idx+1) == dev->width())
			os << "    PIN, I" << idx/5 << ", I, " << mname
			   << "/bit<" << idx << ">" << endl;
		  else
			os << "    PIN, I" << idx/5 << ", I, " << mname
			   << "/nbit<" << idx << ">" << endl;
	    }
	    os << "END" << endl;

      } else {
	    if (dev->pin_AEB().is_linked()) {
		  assert( ! dev->pin_ANEB().is_linked());
		  os << "SYM, " << mname << ", AND, LIBVER=2.0.0" << endl;
		  draw_pin(os, "O", dev->pin_AEB());

	    } else {
		  assert( dev->pin_ANEB().is_linked());
		  os << "SYM, " << mname << ", NAND, LIBVER=2.0.0" << endl;
		  draw_pin(os, "O", dev->pin_ANEB());
	    }

	    for (unsigned idx = 0 ;  idx < dev->width() ;  idx += 1) {
		  os << "    PIN, I" << idx << ", I, " << mname << "/bit<"
		     << idx << ">" << endl;
	    }
	    os << "END" << endl;
      }
}

void target_xnf::lpm_compare_ge_(ostream&os, const NetCompare*dev)
{
      cerr << "XXXX GE not supported yet" << endl;
}

void target_xnf::lpm_compare_le_(ostream&os, const NetCompare*dev)
{
      cerr << "XXXX LE not supported yet" << endl;
}

void target_xnf::lpm_ff(const NetFF*net)
{
      string type = net->attribute(perm_string::literal("LPM_FFType")).as_string();
      if (type == "") type = "DFF";

	// XXXX For now, only support DFF
      assert(type == "DFF");

      string lcaname = net->attribute(perm_string::literal("XNF-LCA")).as_string();
      if (lcaname != "") {
	    draw_sym_with_lcaname(out_, lcaname, net);
	    return;
      }

      assert(net->attribute(perm_string::literal("XNF-LCA")) == verinum(""));

	/* Create a DFF object for each bit of width. The symbol name
	   has the index number appended so that read XNF may be able
	   to buss them. If the NetNet objects connected to the Q
	   output of the DFF have an initial value, the write an INIT=
	   parameter to set the power-up value. */

      for (unsigned idx = 0 ;  idx < net->width() ;  idx += 1) {

	    verinum::V ival = link_get_ival(net->pin_Q(idx));

	    out_ << "SYM, " << mangle(net->name()) << "<" << idx << ">, DFF, ";

	    switch (ival) {
		case verinum::V0:
		  out_ << "INIT=R, ";
		  break;
		case verinum::V1:
		  out_ << "INIT=S, ";
		  break;

		default:
		  break;
	    }

	    out_ << "LIBVER=2.0.0" << endl;
	    draw_pin(out_, "Q", net->pin_Q(idx));
	    draw_pin(out_, "D", net->pin_Data(idx));

	    if (net->attribute(perm_string::literal("Clock:LPM_Polarity")) == verinum("INVERT"))
		  draw_pin(out_, "~C", net->pin_Clock());
	    else
		  draw_pin(out_, "C", net->pin_Clock());

	    if (net->pin_Enable().is_linked())
		  draw_pin(out_, "CE", net->pin_Enable());

	    out_ << "END" << endl;
      }
}

/*
 * Generate an LPM_MUX.
 *
 *   XXXX NOTE: For now, this only supports combinational LPM_MUX
 *   devices that have a single select input. These are typically
 *   generated from ?: expressions.
 */
void target_xnf::lpm_mux(const NetMux*net)
{
      assert(net->sel_width() == 1);
      assert(net->size() == 2);

      for (unsigned idx = 0 ;  idx < net->width() ;  idx += 1) {

	    out_ << "SYM, " << mangle(net->name()) << "<" << idx << ">,"
	       << " EQN, EQN=(I0 * I2) + (~I0 * I1)" << endl;

	    draw_pin(out_, "I0", net->pin_Sel(0));
	    draw_pin(out_, "I1", net->pin_Data(idx,0));
	    draw_pin(out_, "I2", net->pin_Data(idx,1));
	    draw_pin(out_, "O",  net->pin_Result(idx));

	    out_ << "END" << endl;
      }

}

void target_xnf::lpm_ram_dq(const NetRamDq*ram)
{
      assert(ram->count_partners() == 1);

      for (unsigned idx = 0 ;  idx < ram->width() ;  idx += 1) {
	    out_ << "SYM, " << mangle(ram->name())
	       << "<" << idx << ">, RAMS" << endl;

	    draw_pin(out_, "O", ram->pin_Q(idx));
	    draw_pin(out_, "D", ram->pin_Data(idx));
	    draw_pin(out_, "WE", ram->pin_WE());
	    draw_pin(out_, "WCLK", ram->pin_InClock());
	    for (unsigned adr = 0 ;  adr < ram->awidth() ;  adr += 1) {
		  ostringstream tmp;
		  tmp << "A" << adr;
		  draw_pin(out_, tmp.str(), ram->pin_Address(adr));
	    }

	    out_ << "END" << endl;
      }
}

bool target_xnf::net_const(const NetConst*c)
{
      unsigned x_bits = 0;
      for (unsigned idx = 0 ;  idx < c->pin_count() ;  idx += 1) {
	    verinum::V v=c->value(idx);
	    const Link& lnk = c->pin(idx);

	    switch (v) {
		case verinum::V0:
		  out_ << "    PWR, 0, " << choose_sig_name(&lnk) << endl;
		  break;
		case verinum::V1:
		  out_ << "    PWR, 1, " << choose_sig_name(&lnk) << endl;
		  break;
		case verinum::Vz:
		  break;
		default:
		  x_bits += 1;
		  if (x_bits == 1)
			cerr << "xnf: error: Unknown (x) const bit value"
			     << " assigned to " << choose_sig_name(&lnk)
			     << endl;
		  break;
	    }
      }

      return x_bits == 0;
}

/*
 * The logic gates I know so far can be translated directly into XNF
 * standard symbol types. This is a fairly obvious transformation.
 */
void target_xnf::logic(const NetLogic*net)
{
	// The XNF-LCA attribute overrides anything I might guess
	// about this object.
      string lca = net->attribute(perm_string::literal("XNF-LCA")).as_string();
      if (lca != "") {
	    draw_sym_with_lcaname(out_, lca, net);
	    return;
      }

      out_ << "SYM, " << mangle(net->name()) << ", ";
      switch (net->type()) {
	  case NetLogic::AND:
	    out_ << "AND";
	    break;
	  case NetLogic::BUF:
	    out_ << "BUF";
	    break;
	  case NetLogic::NAND:
	    out_ << "NAND";
	    break;
	  case NetLogic::NOR:
	    out_ << "NOR";
	    break;
	  case NetLogic::NOT:
	    out_ << "INV";
	    break;
	  case NetLogic::OR:
	    out_ << "OR";
	    break;
	  case NetLogic::XNOR:
	    out_ << "XNOR";
	    break;
	  case NetLogic::XOR:
	    out_ << "XOR";
	    break;
	  case NetLogic::BUFIF0:
	  case NetLogic::BUFIF1:
	    out_ << "TBUF";
	    break;
	  default:
	    cerr << "internal error: XNF: Unhandled logic type." << endl;
	    break;
      }
      out_ << ", LIBVER=2.0.0" << endl;

	/* All of these kinds of devices have an output on pin 0. */
      draw_pin(out_, "O", net->pin(0));

	/* Most devices have inputs called I<N> for all the remaining
	   pins. The TBUF devices are slightly different, but
	   essentially the same structure. */
      switch (net->type()) {

	  case NetLogic::BUFIF0:
	    assert(net->pin_count() == 3);
	    draw_pin(out_,  "I", net->pin(1));
	    draw_pin(out_, "~T", net->pin(2));
	    break;

	  case NetLogic::BUFIF1:
	    assert(net->pin_count() == 3);
	    draw_pin(out_, "I", net->pin(1));
	    draw_pin(out_, "T", net->pin(2));
	    break;

	  default:
	    if (net->pin_count() == 2) {
		  draw_pin(out_, "I", net->pin(1));
	    } else for (unsigned idx = 1; idx < net->pin_count(); idx += 1) {
		  string name = "I";
		  assert(net->pin_count() <= 11);
		  name += (char)('0'+idx-1);
		  draw_pin(out_, name, net->pin(idx));
	    }
	    break;
      }

      out_ << "END" << endl;
}

bool target_xnf::bufz(const NetBUFZ*net)
{
      static int warned_once=0;
      if (!warned_once) {
	    cerr << "0:0: internal warning: BUFZ objects found "
		 << "in XNF netlist." << endl;
	    cerr << "0:0:                 : I'll make BUFs for them."
		 << endl;
	    warned_once=1;
      }
      out_ << "SYM, " << mangle(net->name()) << ", BUF, LIBVER=2.0.0" << endl;
      assert(net->pin_count() == 2);
      draw_pin(out_, "O", net->pin(0));
      draw_pin(out_, "I", net->pin(1));
      out_ << "END" << endl;

      return true;
}

void target_xnf::udp(const NetUDP*net)
{
      string lca = net->attribute(perm_string::literal("XNF-LCA")).as_string();

	// I only know how to draw a UDP if it has the XNF-LCA
	// attribute attached to it.
      if (lca == "") {
	    cerr << "I don't understand this UDP." << endl;
	    return;
      }

      draw_sym_with_lcaname(out_, lca, net);
}

static target_xnf target_xnf_obj;

extern const struct target tgt_xnf = { "xnf", &target_xnf_obj };

/*
 * $Log: t-xnf.cc,v $
 * Revision 1.52  2004/02/20 18:53:36  steve
 *  Addtrbute keys are perm_strings.
 *
 * Revision 1.51  2004/02/18 17:11:58  steve
 *  Use perm_strings for named langiage items.
 *
 * Revision 1.50  2003/11/10 20:59:04  steve
 *  Design::get_flag returns const char* instead of string.
 *
 * Revision 1.49  2003/07/05 20:42:08  steve
 *  Fix some enumeration warnings.
 *
 * Revision 1.48  2003/06/24 01:38:03  steve
 *  Various warnings fixed.
 *
 * Revision 1.47  2003/01/30 16:23:08  steve
 *  Spelling fixes.
 *
 * Revision 1.46  2003/01/14 21:16:18  steve
 *  Move strstream to ostringstream for compatibility.
 *
 * Revision 1.45  2002/08/12 01:35:01  steve
 *  conditional ident string using autoconfig.
 *
 * Revision 1.44  2002/05/23 03:08:52  steve
 *  Add language support for Verilog-2001 attribute
 *  syntax. Hook this support into existing $attribute
 *  handling, and add number and void value types.
 *
 *  Add to the ivl_target API new functions for access
 *  of complex attributes attached to gates.
 *
 * Revision 1.43  2001/07/25 03:10:50  steve
 *  Create a config.h.in file to hold all the config
 *  junk, and support gcc 3.0. (Stephan Boettcher)
 *
 * Revision 1.42  2001/06/30 20:11:15  steve
 *  typo in CYMODE=EXAMINE-CI string.
 *
 * Revision 1.41  2001/03/27 03:31:06  steve
 *  Support error code from target_t::end_design method.
 *
 * Revision 1.40  2001/02/10 03:22:49  steve
 *  Report errors when XNF code has constant X values. (PR#128)
 *
 * Revision 1.39  2000/11/29 23:15:54  steve
 *  More informative BUFZ warning.
 *
 * Revision 1.38  2000/11/29 02:54:49  steve
 *  Add XNF support for NE comparators.
 *
 * Revision 1.37  2000/11/29 01:34:17  steve
 *  Typo writing I pins to AND gates in compare.
 *
 * Revision 1.36  2000/11/22 21:18:20  steve
 *  Connect the CE if it is linked at all.
 *
 * Revision 1.35  2000/08/14 04:39:57  steve
 *  add th t-dll functions for net_const, net_bufz and processes.
 *
 * Revision 1.34  2000/08/09 03:43:45  steve
 *  Move all file manipulation out of target class.
 *
 * Revision 1.33  2000/08/08 01:50:42  steve
 *  target methods need not take a file stream.
 *
 * Revision 1.32  2000/07/14 06:12:58  steve
 *  Move inital value handling from NetNet to Nexus
 *  objects. This allows better propogation of inital
 *  values.
 *
 *  Clean up constant propagation  a bit to account
 *  for regs that are not really values.
 *
 * Revision 1.31  2000/06/28 18:38:54  steve
 *  Use nexus type to get nexus name.
 *
 * Revision 1.30  2000/06/25 19:59:42  steve
 *  Redesign Links to include the Nexus class that
 *  carries properties of the connected set of links.
 *
 * Revision 1.29  2000/05/14 17:55:04  steve
 *  Support initialization of FF Q value.
 *
 * Revision 1.28  2000/05/08 05:29:43  steve
 *  no need for nobufz functor.
 *
 * Revision 1.27  2000/05/07 04:37:56  steve
 *  Carry strength values from Verilog source to the
 *  pform and netlist for gates.
 *
 *  Change vvm constants to use the driver_t to drive
 *  a constant value. This works better if there are
 *  multiple drivers on a signal.
 *
 * Revision 1.26  2000/04/23 23:03:13  steve
 *  automatically generate macro interface code.
 *
 * Revision 1.25  2000/04/23 21:15:07  steve
 *  Emit code for the bufif devices.
 *
 * Revision 1.24  2000/04/20 02:34:47  steve
 *  Generate code for identity compare. Use gates.
 *
 * Revision 1.23  2000/02/23 02:56:55  steve
 *  Macintosh compilers do not support ident.
 *
 * Revision 1.22  1999/12/17 03:38:46  steve
 *  NetConst can now hold wide constants.
 *
 * Revision 1.21  1999/12/16 18:54:32  steve
 *  Capture the carry out of carry-chain addition.
 *
 * Revision 1.20  1999/12/16 02:42:15  steve
 *  Simulate carry output on adders.
 *
 * Revision 1.19  1999/12/05 19:30:43  steve
 *  Generate XNF RAMS from synthesized memories.
 *
 * Revision 1.18  1999/11/19 03:02:25  steve
 *  Detect flip-flops connected to opads and turn
 *  them into OUTFF devices. Inprove support for
 *  the XNF-LCA attribute in the process.
 *
 * Revision 1.17  1999/11/17 18:52:09  steve
 *  Add algorithm for choosing nexus name from attached signals.
 *
 * Revision 1.16  1999/11/17 01:31:28  steve
 *  Clean up warnings that add_sub got from Alliance
 *
 * Revision 1.15  1999/11/06 04:51:42  steve
 *  Support writing some XNF things into an NCF file.
 *
 * Revision 1.14  1999/11/05 18:43:12  steve
 *  fix syntax of EQN record.
 *
 * Revision 1.13  1999/11/05 07:10:45  steve
 *  Include the obvious XOR gates in the adders.
 *
 * Revision 1.12  1999/11/05 04:40:40  steve
 *  Patch to synthesize LPM_ADD_SUB from expressions,
 *  Thanks to Larry Doolittle. Also handle constants
 *  in expressions.
 *
 *  Synthesize adders in XNF, based on a patch from
 *  Larry. Accept synthesis of constants from Larry
 *  as is.
 *
 * Revision 1.11  1999/11/04 03:53:26  steve
 *  Patch to synthesize unary ~ and the ternary operator.
 *  Thanks to Larry Doolittle <LRDoolittle@lbl.gov>.
 *
 *  Add the LPM_MUX device, and integrate it with the
 *  ternary synthesis from Larry. Replace the lpm_mux
 *  generator in t-xnf.cc to use XNF EQU devices to
 *  put muxs into function units.
 *
 *  Rewrite elaborate_net for the PETernary class to
 *  also use the LPM_MUX device.
 *
 * Revision 1.10  1999/11/02 04:55:34  steve
 *  Add the synthesize method to NetExpr to handle
 *  synthesis of expressions, and use that method
 *  to improve r-value handling of LPM_FF synthesis.
 *
 *  Modify the XNF target to handle LPM_FF objects.
 *
 * Revision 1.9  1999/08/25 22:22:08  steve
 *  handle bufz in XNF backend.
 *
 * Revision 1.8  1999/08/18 04:00:02  steve
 *  Fixup spelling and some error messages. <LRDoolittle@lbl.gov>
 *
 * Revision 1.7  1999/07/17 03:39:11  steve
 *  simplified process scan for targets.
 *
 * Revision 1.6  1998/12/09 02:43:19  steve
 *  Fix 2pin logic gates.
 *
 * Revision 1.5  1998/12/07 04:53:17  steve
 *  Generate OBUF or IBUF attributes (and the gates
 *  to garry them) where a wire is a pad. This involved
 *  figuring out enough of the netlist to know when such
 *  was needed, and to generate new gates and signales
 *  to handle what's missing.
 *
 * Revision 1.4  1998/12/02 04:37:13  steve
 *  Add the nobufz function to eliminate bufz objects,
 *  Object links are marked with direction,
 *  constant propagation is more careful will wide links,
 *  Signal folding is aware of attributes, and
 *  the XNF target can dump UDP objects based on LCA
 *  attributes.
 *
 * Revision 1.3  1998/11/23 00:20:24  steve
 *  NetAssign handles lvalues as pin links
 *  instead of a signal pointer,
 *  Wire attributes added,
 *  Ability to parse UDP descriptions added,
 *  XNF generates EXT records for signals with
 *  the PAD attribute.
 *
 * Revision 1.2  1998/11/18 04:25:22  steve
 *  Add -f flags for generic flag key/values.
 *
 * Revision 1.1  1998/11/16 05:03:53  steve
 *  Add the sigfold function that unlinks excess
 *  signal nodes, and add the XNF target.
 *
 */

