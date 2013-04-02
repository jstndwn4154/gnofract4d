import os
import locale
import gettext

def i18n():
	APP_NAME = 'gnofract4d'

	langs = []
	lc, encoding = locale.getdefaultlocale()
	if (lc):
		langs = [lc]

	language = os.environ.get('LANGUAGE', None)
	if (language):
		langs += language.split(":")

	# Edit this line whenever new languages are added!
	langs += ['en_GB', 'en_US']

	gettext.find(APP_NAME)
	gettext.textdomain(APP_NAME)
	gettext.bind_textdomain_codeset(APP_NAME, 'UTF-8')
	i18n_lang = gettext.translation(domain=APP_NAME, languages=langs, fallback=True, codeset='UTF-8')
	i18n_lang.install(unicode=True)
