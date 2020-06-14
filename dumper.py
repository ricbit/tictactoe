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

def print_board(n, d, board):
  if d == 2:
    print(board)
    x_set = set(p for i, p in enumerate(board) if i % 2 == 0)
    o_set = set(p for i, p in enumerate(board) if i % 2 == 1)
    s = []
    for j in range(0, n):
      for i in range(0, n):
        pos = j * n + i
        if pos in x_set:
          s.append("X")
        elif pos in o_set:
          s.append("O")
        else:
          s.append(".")
      s.append("\n")
    print("".join(s))



def print_tree(tree, node, board, max_depth, depth):
  print_board(tree.n, tree.d, board)
  if depth < max_depth:
    for pos, childnode in node.children.items():
      board.append(pos)
      print_tree(tree, childnode, board, max_depth, depth + 1)
      board.pop()

def main():
  tree = read_file("solution.txt")
  print_tree(tree, tree.root, [], 3, 0)

if __name__ == '__main__':
  main()

