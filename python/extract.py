import commands
import glob
import re
import os
import string
import sys

def clean_func(buf):
    buf = strip_comments(buf)
    pat = re.compile(r"""\\\n""", re.MULTILINE) 
    buf = pat.sub('',buf)
    pat = re.compile(r"""^[#].*?$""", re.MULTILINE) 
    buf = pat.sub('',buf)
    pat = re.compile(r"""^(typedef|struct|enum)(\s|.|\n)*?;\s*""", re.MULTILINE) 
    buf = pat.sub('',buf)
    pat = re.compile(r"""\s+""", re.MULTILINE) 
    buf = pat.sub(' ',buf)
    pat = re.compile(r""";\s*""", re.MULTILINE) 
    buf = pat.sub('\n',buf)
    buf = buf.lstrip()
    #pat=re.compile(r'\s+([*|&]+)\s*(\w+)')
    pat = re.compile(r' \s+ ([*|&]+) \s* (\w+)',re.VERBOSE)
    buf = pat.sub(r'\1 \2', buf)
    pat = re.compile(r'\s+ (\w+) \[ \s* \]',re.VERBOSE)
    buf = pat.sub(r'[] \1', buf)
#    buf = string.replace(buf, 'G_CONST_RETURN ', 'const-')
    buf = string.replace(buf, 'const ', '')
    return buf

def strip_comments(buf):
    parts = []
    lastpos = 0
    while 1:
        pos = string.find(buf, '/*', lastpos)
        if pos >= 0:
            parts.append(buf[lastpos:pos])
            pos = string.find(buf, '*/', pos)
            if pos >= 0:
                lastpos = pos + 2
            else:
                break
        else:
            parts.append(buf[lastpos:])
            break
    return string.join(parts, '')

def find_enums(buf):
    enums = []    
    buf = strip_comments(buf)
    buf = re.sub('\n', ' ', buf)
    
    enum_pat = re.compile(r'enum\s*{([^}]*)}\s*([A-Z][A-Za-z]*)(\s|;)')
    splitter = re.compile(r'\s*,\s', re.MULTILINE)
    pos = 0
    while pos < len(buf):
        m = enum_pat.search(buf, pos)
        if not m: break

        name = m.group(2)
        vals = m.group(1)
        isflags = string.find(vals, '<<') >= 0
        entries = []
        for val in splitter.split(vals):
            if not string.strip(val): continue
            entries.append(string.split(val)[0])
        enums.append((name, isflags, entries))
        
        pos = m.end()
    return enums

#typedef unsigned int   dbus_bool_t;
#typedef struct {
#
# }
#typedef struct FooStruct FooStruct;
# typedef void (* DBusAddWatchFunction)      (DBusWatch      *watch,
#					    void           *data);

def find_typedefs(buf):
    typedefs = []
    buf = re.sub('\n', ' ', strip_comments(buf))
    typedef_pat = re.compile(
        r"""typedef\s*(?P<type>\w*)
            \s*
            ([(]\s*\*\s*(?P<callback>[\w* ]*)[)]|{([^}]*)}|)
            \s*
            (?P<args1>[(](?P<args2>[\s\w*,_]*)[)]|[\w ]*)""",
        re.MULTILINE | re.VERBOSE)
    pat = re.compile(r"""\s+""", re.MULTILINE) 
    pos = 0
    while pos < len(buf):
        m = typedef_pat.search(buf, pos)
        if not m:
            break
        if m.group('type') == 'enum':
            pos = m.end()
            continue
        if m.group('args2') != None:
            args = pat.sub(' ', m.group('args2'))
            
            current = '%s (* %s) (%s)' % (m.group('type'),
                                          m.group('callback'),
                                          args)
        else:
            current = '%s %s' % (m.group('type'), m.group('args1'))
        typedefs.append(current)
        pos = m.end()
    return typedefs

proto_pat = re.compile(r"""
(?P<ret>(-|\w|\&|\*|\s)+\s*)      # return type
\s+                            # skip whitespace
(?P<func>\w+)\s*[(]  # match the function name until the opening (
(?P<args>.*?)[)]               # group the function arguments
""", re.IGNORECASE|re.VERBOSE)
arg_split_pat = re.compile("\s*,\s*")


def find_functions(buf):
    functions = []
    buf = clean_func(buf)
    buf = string.split(buf,'\n')
    for p in buf:
        if len(p) == 0:
            continue
        m = proto_pat.match(p)
        if m == None:
            continue
        
        func = m.group('func')
        ret = m.group('ret')
        args = m.group('args')
        args = arg_split_pat.split(args)
#        for i in range(len(args)):
#            spaces = string.count(args[i], ' ')
#            if spaces > 1:
#                args[i] = string.replace(args[i], ' ', '-', spaces - 1)

        functions.append((func, ret, args))
    return functions

class Writer:
    def __init__(self, filename, enums, typedefs, functions):
        if not (enums or typedefs or functions):
            return
        print 'cdef extern from "%s":' % filename

        self.output_enums(enums)
        self.output_typedefs(typedefs)
        self.output_functions(functions)        
        
        print '    pass'
        print
        
    def output_enums(self, enums):
        for enum in enums:
            print '    ctypedef enum %s:' % enum[0]
            if enum[1] == 0:
                for item in enum[2]:
                    print '        %s' % item
            else:
                i = 0
                for item in enum[2]:
                    print '        %s' % item                    
#                    print '        %s = 1 << %d' % (item, i)
                    i += 1
            print
    def output_typedefs(self, typedefs):
        for typedef in typedefs:
            if typedef.find('va_list') != -1:
                continue
            
            parts = typedef.split()
            if parts[0] == 'struct':
                if parts[-2] == parts[-1]:
                    parts = parts[:-1]
                print '    ctypedef %s' % ' '.join(parts)
            else:
                print '    ctypedef %s' % typedef

    def output_functions(self, functions):
        for func, ret, args in functions:
            if func[0] == '_':
                continue

            str = ', '.join(args)
            if str.find('...') != -1:
                continue
            if str.find('va_list') != -1:
                continue
            if str.strip() == 'void':
                continue
            print '    %-20s %s (%s)' % (ret, func, str)

def do_buffer(name, buffer):
    functions = find_functions(buffer)
    typedefs = find_typedefs(buffer)
    enums = find_enums(buffer)

    Writer(name, enums, typedefs, functions)
    
def do_header(filename, name=None):
    if name == None:
        name = filename
        
    buffer = ""
    for line in open(filename).readlines():
        if line[0] == '#':
            continue
        buffer += line

    print '# -- %s -- ' % filename
    do_buffer(name, buffer)
    
filename = sys.argv[1]

if filename.endswith('.h'):
    do_header(filename)
    raise SystemExit

cppflags = ""

for flag in sys.argv[2:]:
    cppflags = cppflags + " " + flag

fd = open(filename)

for line in fd.readlines():
    if line.startswith('#include'):
        filename = line.split(' ')[1][1:-2]
        command = "echo '%s'|cpp %s" % (line, cppflags)
        sys.stderr.write('running %s' % (command))
        output = commands.getoutput(command)
        do_buffer(filename, output)
    else:
        print line[:-1]
