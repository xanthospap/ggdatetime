///
/// @file  dtfund.cpp
///
/// @brief Implementation file for header dtfund.hpp.
///
/// This file contains fundamental constants and algorithms for manipulating
/// date and time, targeting GNSS applications.
///
/// @author xanthos
///
/// @bug No known bugs.
///

#include "dtfund.hpp"
#include <cassert>

/// Definition for static month array (short names).
/// @see ngpt::month
constexpr const char* ngpt::month::short_names[];

/// Definition for static month array (long names).
/// @see ngpt::month
constexpr const char* ngpt::month::long_names[];

/// Number of days past at the end of non-leap and leap years.
constexpr static long month_day[2][13] = {
  {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
  {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
};

/// Given a calendar date (i.e. year, month and day of month), compute the 
/// corresponding Modified Julian Day. The input date is checked and an 
/// exception is thrown if it is invalid.
///
/// @note The algorithm used is valid from -4800 March 1
constexpr long
ngpt::cal2mjd(int iy, int im, int id)
{
  // Month lengths in days
  constexpr int mtab[] =  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  // Validate month
  if ( im < 1 || im > 12 ) {
    throw std::out_of_range("ngpt::cal2mjd -> Invalid Month.");
  }

  // If February in a leap year, 1, otherwise 0
  int ly = ((im == 2) && !(iy%4) && (iy%100 || !(iy%400)));

  // Validate day, taking into account leap years
  if ( (id < 1) || (id > (mtab[im-1] + ly))) {
    throw std::out_of_range("ngpt::cal2mjd -> Invalid Day of Month.");
  }

  // Compute mjd
  int  my    { (im-14) / 12 };
  long iypmy { static_cast<long>(iy + my) };

  return  (1461L * (iypmy + 4800L)) / 4L
          + (367L * static_cast<long>(im - 2 - 12 * my)) / 12L
          - (3L * ((iypmy + 4900L) / 100L)) / 4L
          + static_cast<long>(id) - 2432076L;
}

///
/// Given a calendar date (i.e. year, month and day of month), compute the 
/// corresponding Modified Julian Day.  
/// Actually, this function is just a wrapper aroung ngpt::cal2mjd, designed
/// to work with datetime classes. The implementation is all performed by
/// ngpt::cal2mjd.
/// This function will only throw if the underlying function (i.e. ngpt::cal2mjd)
/// throws.
///
ngpt::modified_julian_day
ngpt::cal2mjd(ngpt::year y, ngpt::month m, ngpt::day_of_month d)
{
  ngpt::modified_julian_day::underlying_type mjd { 
      ngpt::cal2mjd(y.as_underlying_type(),
      m.as_underlying_type(), 
      d.as_underlying_type()) };

  return ngpt::modified_julian_day{mjd};
}

int
__lower_strncmp__(const char *str1, const char *str2, std::size_t n=0)
{
  if (!n) n = std::max(std::strlen(str1), std::strlen(str2));
  for (std::size_t i = 0; i < n; i++) 
    if ( !(std::tolower(str1[i]) == std::tolower(str2[i])) ) 
      return 1;
  return 0;
}

///
/// Given a c-string (i.e. null-terminating char array), resolve the month.
/// The c-string can be either a short name (i.e. a 3-character name), e.g.
/// "Jan", or the whole, normal month name e.g. "January". 
/// If a 3-char length string is passed in, we are going to compare using the
/// month::short_names array; else if the length of the input string is more
/// than 3-chars, the month::long_names array is used.
/// The comparisson is performed with both strings (aka the input string and
/// each of the long/short_names arrays) beeing already transformed to lowercase
/// That means that this function is actually case-insensitive!
/// If the input string cannot be matced to any of the strings in short_names
/// and long_names, then an exception is thrown of type: std::invalid_argument
/// Note that the month will be returned in the "normal" range [1,12], 
/// **not** [0-11].
///
ngpt::month::month(const char* str)
{
  m_month = 0;

  if ( std::strlen(str) == 3 ) {
    for (int i = 0; i < 12; ++i) {
      if ( !__lower_strncmp__(short_names[i], str, 3) ) {
        m_month = i+1;
        break;
      }
    }
  } else if ( std::strlen(str) > 3 ) {
    for (int i = 0; i < 12; ++i) {
      if ( !__lower_strncmp__(long_names[i], str) ) {
        m_month = i+1;
        break;
      }
    }
  }

  if (!m_month || std::strlen(str) < 3 ) {
    throw std::invalid_argument("Failed to set month from string \""
      +std::string(str)+"\"");
  }
}

///
/// Validate a given day_of_month. To do this, we obviously need the month the
/// dom refers to (to see how many day the month actualy has) and the year, to
/// check if it is leap or not.
///
bool
ngpt::day_of_month::is_valid(ngpt::year y, ngpt::month m) const noexcept
{
  if (m_dom <=0 || m_dom >= 32) return false;
  if (!m.is_valid()) return false;

  auto iy = y.as_underlying_type();
  auto im = m.as_underlying_type();

  // Month lengths in days
  constexpr int mtab[] =  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  // If February in a leap year, 1, otherwise 0
  int ly ( (im == 2) && is_leap(iy) );

  // Validate day, taking into account leap years
  return (m_dom <= mtab[im-1] + ly);
}

///
/// Given a modified_julian_day convert it to a tuple (i.e. a pair) of
/// year and day_of_year. 
///
ngpt::ydoy_date
ngpt::modified_julian_day::to_ydoy() const noexcept
{
  long days_fr_jan1_1901 { m_mjd - ngpt::jan11901 };
  long num_four_yrs      { days_fr_jan1_1901/1461L };
  long years_so_far      { 1901L + 4*num_four_yrs };
  long days_left         { days_fr_jan1_1901 - 1461*num_four_yrs };
  long delta_yrs         { days_left/365 - days_left/1460 };
  
  ydoy_date ydoy;
  ydoy.__doy = day_of_year{static_cast<day_of_year::underlying_type>
              (days_left-365*delta_yrs+1)};
  ydoy.__year= year{static_cast<year::underlying_type>
              (years_so_far + delta_yrs)};

  return ydoy;
}

///
/// Given a modified_julian_day convert it to a calendar date, i.e. a tuple
/// containing (year, month, day_of_month). Calendar date is Gregorian.
///
ngpt::ymd_date
ngpt::modified_julian_day::to_ymd() const noexcept
{
  ymd_date ymd;
  
  // Express day in Gregorian calendar
  long l = m_mjd + (68569L + 2400000L + 1);
  long n = (4L * l) / 146097L;
  l -= (146097L * n + 3L) / 4L;
  long i = (4000L * (l + 1L)) / 1461001L;
  l -= (1461L * i) / 4L - 31L;
  long k = (80L * l) / 2447L;
  ymd.__dom = day_of_month{ static_cast<day_of_month::underlying_type>
                            (l - (2447L * k) / 80L) };
  l = k / 11L;
  ymd.__month = month{ static_cast<month::underlying_type>
                       (k + 2L - 12L * l) };
  ymd.__year  = year { static_cast<year::underlying_type>
                      (100L * (n - 49L) + i + l) };

  return ymd;
}

///
/// Convert a pair of year, day_of_year to a modified_julian_day. No check is
/// performed whatsoever, at the input arguments (e.g. to see if indeed 
/// the given doy is within a valid range).
///
ngpt::modified_julian_day
ngpt::ydoy2mjd(ngpt::year yr, ngpt::day_of_year doy) noexcept
{
    long iyr {static_cast<long>(yr.as_underlying_type())};
    long idy {static_cast<long>(doy.as_underlying_type())};

    return modified_julian_day {((iyr-1901)/4)*1461 + ((iyr-1901)%4)*365 +
        idy - 1 + ngpt::jan11901};
}

/// 
ngpt::ydoy_date
ngpt::ymd_date::to_ydoy() const noexcept
{
  ydoy_date yd;
  yd.__year = __year;
  int leap  = __year.is_leap();
  int md    = __month.as_underlying_type() - 1;
#ifdef DEBUG
  assert( md >= 0 && md < 12 );
#endif
  yd.__doy  = month_day[leap][md] + __dom.as_underlying_type();
  return yd;
}

ngpt::ymd_date
ngpt::ydoy_date::to_ymd() const noexcept
{
  ymd_date yd;
  yd.__year = __year;
  int guess = __doy.as_underlying_type() * 0.032;
  int leap = __year.is_leap();
#ifdef DEBUG
  assert( guess >= 0 && guess < 11 );
#endif
  int more = (( __doy.as_underlying_type() - month_day[leap][guess+1] ) > 0);
  yd.__month = month{guess + more + 1};
#ifdef DEBUG
  assert( guess+more >= 0 && guess+more < 12 );
#endif
  yd.__dom   = day_of_month(__doy.as_underlying_type()
                           - month_day[leap][guess+more]);
  return yd;
}
