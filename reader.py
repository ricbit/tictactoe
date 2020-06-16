import re

class Node:
  def __init__(self):
    self.children = {}
  def __str__(self):
    return "%d %d" % (self.result, self.count)

class Tree:
  def __init__(self, n, d, root):
    self.n = n
    self.d = d
    self.root = root

parser = re.compile(r"(\d+) (\d+) (\d+) :(.*)$")

def read_line(f, node):
  line = f.readline()
  p = parser.match(line)
  (result, count, size, children) = [p.group(x) for x in range(1, 5)]  
  node.result = int(result)
  node.count = int(count)
  for childpos in map(int, children.split()):
    child_node = Node()
    read_line(f, child_node)
    node.children[childpos] = child_node

def read_file(filename):
  f = open(filename, "rt")
  n, d = map(int, f.readline().split())
  root = Node()
  read_line(f, root)
  return Tree(n, d, root)


