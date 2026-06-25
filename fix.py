with open('src/main.cpp', 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()
idx = next(i for i, l in enumerate(lines) if 'CRAWL FINISHED' in l)
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.writelines(lines[:idx])
    f.write('    std::cout << "\\n[RESULTS]" << std::endl;\n')
    f.write('    return 0;\n')
    f.write('}\n')
