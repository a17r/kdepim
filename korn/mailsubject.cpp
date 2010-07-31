#include"mailsubject.h"

#include <kmime_codecs.h>
#include <kcharsets.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <tqdatetime.h>
#include <tqtextcodec.h>
#include <ctype.h>

KornMailSubject::KornMailSubject() : _id(0), _drop(0), _size(-1), _date(-1), _fullMessage(false)
{
}

KornMailSubject::KornMailSubject(KornMailId * id, KMailDrop *drop)
	: _id(id), _drop( drop ), _size(-1), _date(-1), _fullMessage(false)
{
}

KornMailSubject::KornMailSubject(const KornMailSubject & src)
	: _id(0), _drop(0), _size(-1), _date(-1)
{
	operator=(src);
}

KornMailSubject & KornMailSubject::operator= (const KornMailSubject & src)
{
	_size = src._size;
	_date = src._date;
	_subject = src._subject;
	_sender = src._sender;
	_header = src._header;
	_fullMessage = src._fullMessage;
	if (_id)
		delete _id;
	_id = 0;
	if (src._id)
		_id = src._id->clone();
	_drop = src._drop;
	return *this;
}

KornMailSubject::~KornMailSubject()
{
	if (_id)
		delete _id;
	_id = 0;
}

TQString KornMailSubject::toString() const
{
	TQDateTime date;
	date.setTime_t(_date);
	return TQString("KornMailSubject, Id: ") + (_id?_id->toString():TQString("NULL")) + ", " + i18n("Subject:") + " " + _subject
		+ ", " + i18n("Sender:") + " " + _sender + ", " + i18n("Size:") + " " + TQString::number(_size)
		+ ", " + i18n("Date:") + " " + date.toString(Qt::ISODate);
}

TQString KornMailSubject::decodeRFC2047String(const TQCString& aStr)
{
	if ( aStr.isEmpty() )
		return TQString::null;

	const TQCString str = unfold( aStr );

	if ( str.isEmpty() )
		return TQString::null;

	if ( str.find( "=?" ) < 0 ) {
		// No need to decode
		return TQString(str);
	}

	TQString result;
	TQCString LWSP_buffer;
	bool lastWasEncodedWord = false;

	for ( const char * pos = str.data() ; *pos ; ++pos ) {
		// collect LWSP after encoded-words,
		// because we might need to throw it out
		// (when the next word is an encoded-word)
		if ( lastWasEncodedWord && isBlank( pos[0] ) ) {
			LWSP_buffer += pos[0];
			continue;
		}
		// verbatimly copy normal text
		if (pos[0]!='=' || pos[1]!='?') {
			result += LWSP_buffer + pos[0];
			LWSP_buffer = 0;
			lastWasEncodedWord = false;
			continue;
		}
		// found possible encoded-word
		const char * const beg = pos;
		{
			// parse charset name
			TQCString charset;
			int i = 2;
			pos += 2;
			for ( ; *pos != '?' && ( *pos==' ' || ispunct(*pos) || isalnum(*pos) ); ++i, ++pos ) {
				charset += *pos;
			}
			if ( *pos!='?' || i<4 )
				goto invalid_encoded_word;

			// get encoding and check delimiting question marks
			const char encoding[2] = { pos[1], '\0' };
			if (pos[2]!='?' || (encoding[0]!='Q' && encoding[0]!='q' &&
			    encoding[0]!='B' && encoding[0]!='b'))
				goto invalid_encoded_word;
			pos+=3; i+=3; // skip ?x?
			const char * enc_start = pos;
			// search for end of encoded part
			while ( *pos && !(*pos=='?' && *(pos+1)=='=') ) {
				i++;
				pos++;
			}
			if ( !*pos )
				goto invalid_encoded_word;

			// valid encoding: decode and throw away separating LWSP
			const KMime::Codec * c = KMime::Codec::codecForName( encoding );
			kdFatal( !c ) << "No \"" << encoding << "\" codec!?" << endl;

			TQByteArray in; in.setRawData( enc_start, pos - enc_start );
			const TQByteArray enc = c->decode( in );
			in.resetRawData( enc_start, pos - enc_start );

			const TQTextCodec * codec = codecForName(charset);
			if (!codec) return TQString::null;
			result += codec->toUnicode(enc);
			lastWasEncodedWord = true;

			++pos; // eat '?' (for loop eats '=')
			LWSP_buffer = 0;
		}
		continue;
invalid_encoded_word:
		// invalid encoding, keep separating LWSP.
		pos = beg;
		if ( !LWSP_buffer.isNull() )
		result += LWSP_buffer;
		result += "=?";
		lastWasEncodedWord = false;
		++pos; // eat '?' (for loop eats '=')
		LWSP_buffer = 0;
	}
	return result;
}

TQCString KornMailSubject::unfold( const TQCString & header )
{
	if ( header.isEmpty() )
		return TQCString();

	TQCString result( header.size() ); // size() >= length()+1 and size() is O(1)
	char * d = result.data();

	for ( const char * s = header.data() ; *s ; )
		if ( *s == '\r' ) { // ignore
			++s;
			continue;
		} else if ( *s == '\n' ) { // unfold
			while ( this->isBlank( *++s ) );
			*d++ = ' ';
		} else
			*d++ = *s++;

	*d++ = '\0';

	result.truncate( d - result.data() );
	return result;
}

//-----------------------------------------------------------------------------
const TQTextCodec* KornMailSubject::codecForName(const TQCString& _str)
{
	if (_str.isEmpty()) return 0;
		TQCString codec = _str;
	return KGlobal::charsets()->codecForName(codec);
}

void KornMailSubject::decodeHeaders()
{
	_subject = decodeRFC2047String( _subject.latin1() );
	_sender = decodeRFC2047String( _sender.latin1() );
}

