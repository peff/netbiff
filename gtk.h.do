target
dependon
formake <<'EOF'
(if pkg-config gtk+-2.0; then echo '#define GUI_GTK'; fi) >gtk.h
EOF
