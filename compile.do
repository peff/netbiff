target;
dependon('warn-auto.sh', 'conf-cc', 'cflags');
formake <<'EOF';
( cat warn-auto.sh; \
echo exec "`head -1 conf-cc`" "`cat cflags`" '-c $${1+"$$@"}' \
) > compile
chmod 755 compile
EOF
