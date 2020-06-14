import re

tree = []

def read_line(f, node, pos):
  global tree
  line = f.readline()
  p = re.match(r"(\d+) (\d+) (\d+) :(.*)$", line)
  (result, count, size, children) = [p.group(x) for x in range(1, 5)]
  node[0] = (pos, (result, count, size))
  print(tree)
  child = list(map(int, children.split()))
  if child:
    for childpos in child:
      node[1].append([(childpos,()), []])
      a = read_line(f, node[1][-1], pos)
      node[1][2].append(a)
    

def read_file(filename):
  global tree
  f = open(filename, "rt")
  n, d = map(int, f.readline().split())
  tree = [[[(), ()], []]]
  read_line(f, tree[0], None)

def main():
  read_file("solution.txt")

if __name__ == '__main__':
  main()

