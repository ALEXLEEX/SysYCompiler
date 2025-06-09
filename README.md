这是浙江大学编译原理的课程项目。本项目使用C++实现了一个简化版的SysY语言的编译器，将源语言编译成Riscv汇编代码。
网站：[https://compiler.pages.zjusct.io/sp25/]
使用方法：进入根目录，运行make，在根目录下生成可执行文件compiler。
`./compiler path_to_source_file/source_file.sy path_to_target_file`
会将对应的sy源文件编译成汇编代码并在目标文件夹下创建.S文件
