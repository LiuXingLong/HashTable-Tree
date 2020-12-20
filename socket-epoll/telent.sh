#扫描端口
for i in {1..65536}; do (echo 'q')|telnet -e 'q' 127.0.0.1 ${i} >/dev/null 2>&1 && echo -e "\033[31;40m${i}\033[0m\tis open"; done