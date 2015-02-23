
#include "../contactfields.h"

#include <qtest.h>

#include <QtCore/QObject>

class ContactFieldsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testFieldCount();
    void testSetGet();

private:
    KContacts::Addressee mContact;
};

QTEST_MAIN(ContactFieldsTest)

static const QString s_formattedName(QStringLiteral("User, Joe"));
static const QString s_prefix(QStringLiteral("Mr."));
static const QString s_givenName(QStringLiteral("Joe"));
static const QString s_additionalName(QStringLiteral("Doe"));
static const QString s_familyName(QStringLiteral("User"));
static const QString s_suffix(QStringLiteral("Jr."));
static const QString s_nickName(QStringLiteral("joe"));
static const QString s_birthday(QStringLiteral("1966-12-03"));
static const QString s_anniversary(QStringLiteral("1980-10-02"));
static const QString s_homeAddressStreet(QStringLiteral("My Home Street"));
static const QString s_homeAddressPostOfficeBox(QStringLiteral("My Home POB"));
static const QString s_homeAddressLocality(QStringLiteral("My Home Locality"));
static const QString s_homeAddressRegion(QStringLiteral("My Home Address"));
static const QString s_homeAddressPostalCode(QStringLiteral("My Home Postal Code"));
static const QString s_homeAddressCountry(QStringLiteral("My Home Country"));
static const QString s_homeAddressLabel(QStringLiteral("My Home Label"));
static const QString s_businessAddressStreet(QStringLiteral("My Business Street"));
static const QString s_businessAddressPostOfficeBox(QStringLiteral("My Business POB"));
static const QString s_businessAddressLocality(QStringLiteral("My Business Locality"));
static const QString s_businessAddressRegion(QStringLiteral("My Business Region"));
static const QString s_businessAddressPostalCode(QStringLiteral("My Business Postal Code"));
static const QString s_businessAddressCountry(QStringLiteral("My Business Country"));
static const QString s_businessAddressLabel(QStringLiteral("My Business Label"));
static const QString s_homePhone(QStringLiteral("000111222"));
static const QString s_businessPhone(QStringLiteral("333444555"));
static const QString s_mobilePhone(QStringLiteral("666777888"));
static const QString s_homeFax(QStringLiteral("999000111"));
static const QString s_businessFax(QStringLiteral("222333444"));
static const QString s_carPhone(QStringLiteral("555666777"));
static const QString s_isdn(QStringLiteral("888999000"));
static const QString s_pager(QStringLiteral("111222333"));
static const QString s_preferredEmail(QStringLiteral("me@somewhere.org"));
static const QString s_email2(QStringLiteral("you@somewhere.org"));
static const QString s_email3(QStringLiteral("she@somewhere.org"));
static const QString s_email4(QStringLiteral("it@somewhere.org"));
static const QString s_mailer(QStringLiteral("kmail2"));
static const QString s_title(QStringLiteral("Chief"));
static const QString s_role(QStringLiteral("Developer"));
static const QString s_organization(QStringLiteral("KDE"));
static const QString s_note(QStringLiteral("That's a small note"));
static const QString s_homepage(QStringLiteral("http://www.kde.de"));
static const QString s_blogFeed(QStringLiteral("http://planetkde.org"));
static const QString s_profession(QStringLiteral("Developer"));
static const QString s_office(QStringLiteral("Room 2443"));
static const QString s_manager(QStringLiteral("Hans"));
static const QString s_assistant(QStringLiteral("Hins"));
static const QString s_spouse(QStringLiteral("My Darling"));

void ContactFieldsTest::testFieldCount()
{
    QCOMPARE(ContactFields::allFields().count(), 48);
}

void ContactFieldsTest::testSetGet()
{
    KContacts::Addressee contact;

    ContactFields::setValue(ContactFields::FormattedName, s_formattedName, contact);
    ContactFields::setValue(ContactFields::Prefix, s_prefix, contact);
    ContactFields::setValue(ContactFields::GivenName, s_givenName, contact);
    ContactFields::setValue(ContactFields::AdditionalName, s_additionalName, contact);
    ContactFields::setValue(ContactFields::FamilyName, s_familyName, contact);
    ContactFields::setValue(ContactFields::Suffix, s_suffix, contact);
    ContactFields::setValue(ContactFields::NickName, s_nickName, contact);
    ContactFields::setValue(ContactFields::Birthday, s_birthday, contact);
    ContactFields::setValue(ContactFields::Anniversary, s_anniversary, contact);
    ContactFields::setValue(ContactFields::HomeAddressStreet, s_homeAddressStreet, contact);
    ContactFields::setValue(ContactFields::HomeAddressPostOfficeBox, s_homeAddressPostOfficeBox, contact);
    ContactFields::setValue(ContactFields::HomeAddressLocality, s_homeAddressLocality, contact);
    ContactFields::setValue(ContactFields::HomeAddressRegion, s_homeAddressRegion, contact);
    ContactFields::setValue(ContactFields::HomeAddressPostalCode, s_homeAddressPostalCode, contact);
    ContactFields::setValue(ContactFields::HomeAddressCountry, s_homeAddressCountry, contact);
    ContactFields::setValue(ContactFields::HomeAddressLabel, s_homeAddressLabel, contact);
    ContactFields::setValue(ContactFields::BusinessAddressStreet, s_businessAddressStreet, contact);
    ContactFields::setValue(ContactFields::BusinessAddressPostOfficeBox, s_businessAddressPostOfficeBox, contact);
    ContactFields::setValue(ContactFields::BusinessAddressLocality, s_businessAddressLocality, contact);
    ContactFields::setValue(ContactFields::BusinessAddressRegion, s_businessAddressRegion, contact);
    ContactFields::setValue(ContactFields::BusinessAddressPostalCode, s_businessAddressPostalCode, contact);
    ContactFields::setValue(ContactFields::BusinessAddressCountry, s_businessAddressCountry, contact);
    ContactFields::setValue(ContactFields::BusinessAddressLabel, s_businessAddressLabel, contact);
    ContactFields::setValue(ContactFields::HomePhone, s_homePhone, contact);
    ContactFields::setValue(ContactFields::BusinessPhone, s_businessPhone, contact);
    ContactFields::setValue(ContactFields::MobilePhone, s_mobilePhone, contact);
    ContactFields::setValue(ContactFields::HomeFax, s_homeFax, contact);
    ContactFields::setValue(ContactFields::BusinessFax, s_businessFax, contact);
    ContactFields::setValue(ContactFields::CarPhone, s_carPhone, contact);
    ContactFields::setValue(ContactFields::Isdn, s_isdn, contact);
    ContactFields::setValue(ContactFields::Pager, s_pager, contact);
    ContactFields::setValue(ContactFields::PreferredEmail, s_preferredEmail, contact);
    ContactFields::setValue(ContactFields::Email2, s_email2, contact);
    ContactFields::setValue(ContactFields::Email3, s_email3, contact);
    ContactFields::setValue(ContactFields::Email4, s_email4, contact);
    ContactFields::setValue(ContactFields::Mailer, s_mailer, contact);
    ContactFields::setValue(ContactFields::Title, s_title, contact);
    ContactFields::setValue(ContactFields::Role, s_role, contact);
    ContactFields::setValue(ContactFields::Organization, s_organization, contact);
    ContactFields::setValue(ContactFields::Note, s_note, contact);
    ContactFields::setValue(ContactFields::Homepage, s_homepage, contact);
    ContactFields::setValue(ContactFields::BlogFeed, s_blogFeed, contact);
    ContactFields::setValue(ContactFields::Profession, s_profession, contact);
    ContactFields::setValue(ContactFields::Office, s_office, contact);
    ContactFields::setValue(ContactFields::Manager, s_manager, contact);
    ContactFields::setValue(ContactFields::Assistant, s_assistant, contact);
    ContactFields::setValue(ContactFields::Anniversary, s_anniversary, contact);
    ContactFields::setValue(ContactFields::Spouse, s_spouse, contact);

    const KContacts::Addressee contactCopy = contact;

    QCOMPARE(ContactFields::value(ContactFields::FormattedName, contactCopy), s_formattedName);
    QCOMPARE(ContactFields::value(ContactFields::Prefix, contactCopy), s_prefix);
    QCOMPARE(ContactFields::value(ContactFields::GivenName, contactCopy), s_givenName);
    QCOMPARE(ContactFields::value(ContactFields::AdditionalName, contactCopy), s_additionalName);
    QCOMPARE(ContactFields::value(ContactFields::FamilyName, contactCopy), s_familyName);
    QCOMPARE(ContactFields::value(ContactFields::Suffix, contactCopy), s_suffix);
    QCOMPARE(ContactFields::value(ContactFields::NickName, contactCopy), s_nickName);
    QCOMPARE(ContactFields::value(ContactFields::Birthday, contactCopy), s_birthday);
    QCOMPARE(ContactFields::value(ContactFields::Anniversary, contactCopy), s_anniversary);
    QCOMPARE(ContactFields::value(ContactFields::HomeAddressStreet, contactCopy), s_homeAddressStreet);
    QCOMPARE(ContactFields::value(ContactFields::HomeAddressPostOfficeBox, contactCopy), s_homeAddressPostOfficeBox);
    QCOMPARE(ContactFields::value(ContactFields::HomeAddressLocality, contactCopy), s_homeAddressLocality);
    QCOMPARE(ContactFields::value(ContactFields::HomeAddressRegion, contactCopy), s_homeAddressRegion);
    QCOMPARE(ContactFields::value(ContactFields::HomeAddressPostalCode, contactCopy), s_homeAddressPostalCode);
    QCOMPARE(ContactFields::value(ContactFields::HomeAddressCountry, contactCopy), s_homeAddressCountry);
    QCOMPARE(ContactFields::value(ContactFields::HomeAddressLabel, contactCopy), s_homeAddressLabel);
    QCOMPARE(ContactFields::value(ContactFields::BusinessAddressStreet, contactCopy), s_businessAddressStreet);
    QCOMPARE(ContactFields::value(ContactFields::BusinessAddressPostOfficeBox, contactCopy), s_businessAddressPostOfficeBox);
    QCOMPARE(ContactFields::value(ContactFields::BusinessAddressLocality, contactCopy), s_businessAddressLocality);
    QCOMPARE(ContactFields::value(ContactFields::BusinessAddressRegion, contactCopy), s_businessAddressRegion);
    QCOMPARE(ContactFields::value(ContactFields::BusinessAddressPostalCode, contactCopy), s_businessAddressPostalCode);
    QCOMPARE(ContactFields::value(ContactFields::BusinessAddressCountry, contactCopy), s_businessAddressCountry);
    QCOMPARE(ContactFields::value(ContactFields::BusinessAddressLabel, contactCopy), s_businessAddressLabel);
    QCOMPARE(ContactFields::value(ContactFields::HomePhone, contactCopy), s_homePhone);
    QCOMPARE(ContactFields::value(ContactFields::BusinessPhone, contactCopy), s_businessPhone);
    QCOMPARE(ContactFields::value(ContactFields::MobilePhone, contactCopy), s_mobilePhone);
    QCOMPARE(ContactFields::value(ContactFields::HomeFax, contactCopy), s_homeFax);
    QCOMPARE(ContactFields::value(ContactFields::BusinessFax, contactCopy), s_businessFax);
    QCOMPARE(ContactFields::value(ContactFields::CarPhone, contactCopy), s_carPhone);
    QCOMPARE(ContactFields::value(ContactFields::Isdn, contactCopy), s_isdn);
    QCOMPARE(ContactFields::value(ContactFields::Pager, contactCopy), s_pager);
    QCOMPARE(ContactFields::value(ContactFields::PreferredEmail, contactCopy), s_preferredEmail);
    QCOMPARE(ContactFields::value(ContactFields::Email2, contactCopy), s_email2);
    QCOMPARE(ContactFields::value(ContactFields::Email3, contactCopy), s_email3);
    QCOMPARE(ContactFields::value(ContactFields::Email4, contactCopy), s_email4);
    QCOMPARE(ContactFields::value(ContactFields::Mailer, contactCopy), s_mailer);
    QCOMPARE(ContactFields::value(ContactFields::Title, contactCopy), s_title);
    QCOMPARE(ContactFields::value(ContactFields::Role, contactCopy), s_role);
    QCOMPARE(ContactFields::value(ContactFields::Organization, contactCopy), s_organization);
    QCOMPARE(ContactFields::value(ContactFields::Note, contactCopy), s_note);
    QCOMPARE(ContactFields::value(ContactFields::Homepage, contactCopy), s_homepage);
    QCOMPARE(ContactFields::value(ContactFields::BlogFeed, contactCopy), s_blogFeed);
    QCOMPARE(ContactFields::value(ContactFields::Profession, contactCopy), s_profession);
    QCOMPARE(ContactFields::value(ContactFields::Office, contactCopy), s_office);
    QCOMPARE(ContactFields::value(ContactFields::Manager, contactCopy), s_manager);
    QCOMPARE(ContactFields::value(ContactFields::Assistant, contactCopy), s_assistant);
    QCOMPARE(ContactFields::value(ContactFields::Anniversary, contactCopy), s_anniversary);
    QCOMPARE(ContactFields::value(ContactFields::Spouse, contactCopy), s_spouse);
}

#include "contactfieldstest.moc"
