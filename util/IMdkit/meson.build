libimdkit_sources = files(
  'FrameMgr.c',
  'IMConn.c',
  'IMMethod.c',
  'IMValues.c',
  'i18nAttr.c',
  'i18nClbk.c',
  'i18nIMProto.c',
  'i18nIc.c',
  'i18nMethod.c',
  'i18nOffsetCache.c',
  'i18nPtHdr.c',
  'i18nUtil.c',
  'i18nX.c',
)

libimdkit_depends = [
  x11_dep,
]

libimdkit = static_library('IMdkit',
  libimdkit_sources,
  dependencies: libimdkit_depends,
)

libimdkit_dep = declare_dependency(
  link_with: libimdkit,
  include_directories: include_directories('.'),
)
