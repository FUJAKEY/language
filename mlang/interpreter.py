import sys

class MoyLangInterpreter:
    def __init__(self):
        self.variables = {}
        self.indent = 0
        self.compiled_lines = []

    def compile_line(self, line: str):
        line = line.strip()
        if not line or line.startswith('#'):
            return
        tokens = line.split()
        cmd = tokens[0]
        if cmd == 'let':
            if len(tokens) < 4 or tokens[2] != '=':
                raise SyntaxError(f"Invalid let syntax: {line}")
            var = tokens[1]
            expr = ' '.join(tokens[3:])
            self.compiled_lines.append(' ' * self.indent + f"{var} = {expr}")
        elif cmd == 'print':
            expr = ' '.join(tokens[1:])
            self.compiled_lines.append(' ' * self.indent + f"print({expr})")
        elif cmd == 'input':
            var = tokens[1]
            prompt = ' '.join(tokens[2:]) if len(tokens) > 2 else ''
            self.compiled_lines.append(' ' * self.indent + f"{var} = input({prompt})")
        elif cmd == 'if':
            condition = ' '.join(tokens[1:])
            self.compiled_lines.append(' ' * self.indent + f"if {condition}:")
            self.indent += 4
        elif cmd == 'while':
            condition = ' '.join(tokens[1:])
            self.compiled_lines.append(' ' * self.indent + f"while {condition}:")
            self.indent += 4
        elif cmd == 'end':
            self.indent = max(self.indent - 4, 0)
        else:
            # treat as raw python
            self.compiled_lines.append(' ' * self.indent + line)

    def run(self, path: str):
        with open(path, 'r') as f:
            for line in f:
                self.compile_line(line)
        code = '\n'.join(self.compiled_lines)
        exec(code, {})


def main():
    if len(sys.argv) != 2:
        print('Usage: python3 interpreter.py <file.ml>')
        return
    interpreter = MoyLangInterpreter()
    interpreter.run(sys.argv[1])


if __name__ == '__main__':
    main()
