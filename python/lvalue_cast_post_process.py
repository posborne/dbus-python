#!/bin/env python

import re
import sys

exp_pattern = re.compile('(.*)=(.*);')
lval_pyobject_pattern = re.compile('\s*\(\((PyObject[ ]?\*)\)([A-Za-z0-9_ ]+)\)')
lval_structcast_pattern = re.compile('\s*\((struct [A-Za-z0-9_]+ \*)\)([A-Za-z0-9_]+)\-\>([A-Za-z0-9_]+)')

def parse_expression(exp):
	exp_match = exp_pattern.match(exp)
	if exp_match:
		lvalue = exp_match.group(1)
		rvalue = exp_match.group(2)

		lval_match = lval_pyobject_pattern.match(lvalue)

		if lval_match:
			cast = lval_match.group(1)
			lvar = lval_match.group(2)

			return "%s = (%s)(%s);" % (lvar, cast, rvalue)
		else:
			lval_match = lval_structcast_pattern.match(lvalue)
			if lval_match:
				cast = lval_match.group(1)
				casted_var = lval_match.group(2)
				member_var = lval_match.group(3)

				result = "%s->%s = ((%s)%s);" % (
					    casted_var, 
					    member_var, 
					    cast,
					    rvalue)

				return result

	return None

def main():
	if len(sys.argv) != 2:
		print "USAGE: " + sys.argv[0] + " <file name>" 
		return(-1)	

	file = sys.argv[1]
	f = open(file)
	gcc4fix_filename = file + ".gcc4fix"
	outputf = open(gcc4fix_filename, 'w')

	lines = f.readlines()
	f.close()
	for line in lines:
		c = line.count(";")
		if c == 0:
			outputf.write(line)
			continue

		exprs = line.split(';')
		line = ""
		last = exprs.pop()
		for expr in exprs:
			expr = expr + ";"

			result = parse_expression(expr)
			if result:
				line = line + result
			else:
				line = line + expr

		if (last.strip()!=''):
			line = line + last
		else:
			line = line + "\n"
			
		outputf.write(line)
	
	outputf.close()

if __name__ == "__main__":
	sys.exit(main())
