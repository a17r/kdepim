#include <kregexp3.h>
#include <kdebug.h>
#include <kinstance.h>

int
main()
{
  KInstance app("# ");

  // test for http://bugs.kde.org/show_bug.cgi?id=54886
  KRegExp3 reg("^");
  TQString res = reg.replace(TQString::fromLatin1("Fun stuff"),
			    TQString::fromLatin1("[fun] "));
  kdDebug() << res << endl;

}
