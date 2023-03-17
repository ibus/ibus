foreach size : [ 16, 22, 24, 32, 48, ]
  size_str = '@0@x@0@'.format(size)

  install_data(size.to_string() / 'ibus-keyboard.png',
    install_dir: get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
  )
endforeach

install_data(
  'scalable' / 'ibus.svg',
  'scalable' / 'ibus-setup.svg',
  'scalable' / 'ibus-engine.svg',
  'scalable' / 'ibus-keyboard.svg',
  install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)
