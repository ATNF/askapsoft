/// @brief The observation date (Posix Date-time)
/// UCD: 
#pragma db index
#pragma db null
boost::posix_time::ptime observation_date;

/// @brief The HEALPix index of this component
/// UCD: 
#pragma db index
#pragma db not_null
boost::int64_t healpix_index;

/// @brief Scheduling Block identifier
/// UCD: 
#pragma db index
#pragma db null
boost::int64_t sb_id;

/// @brief Component identifier
/// UCD: meta.id;meta.main
#pragma db null
std::string component_id;

/// @brief J2000 right ascension (deg)
/// UCD: pos.eq.ra;meta.main
#pragma db not_null
double ra;

/// @brief J2000 declination (deg)
/// UCD: pos.eq.dec;meta.main
#pragma db not_null
double dec;

/// @brief Error in Right Ascension (arcsec)
/// UCD: stat.error;pos.eq.ra
#pragma db not_null
float ra_err;

/// @brief Error in Declination (arcsec)
/// UCD: stat.error;pos.eq.dec
#pragma db not_null
float dec_err;

/// @brief Frequency (MHz)
/// UCD: em.freq
#pragma db not_null
float freq;

/// @brief Peak flux density (mJy/beam)
/// UCD: phot.flux.density;stat.max;em.radio;stat.fit
#pragma db not_null
float flux_peak;

/// @brief Error in peak flux density (mJy/beam)
/// UCD: stat.error;phot.flux.density;stat.max;em.radio;stat.fit
#pragma db not_null
float flux_peak_err;

/// @brief Integrated flux density (mJy)
/// UCD: phot.flux.density;em.radio;stat.fit
#pragma db not_null
float flux_int;

/// @brief Error in integrated flux density (mJy)
/// UCD: stat.error;phot.flux.density;em.radio;stat.fit
#pragma db not_null
float flux_int_err;

/// @brief FWHM major axis before deconvolution (arcsec)
/// UCD: phys.angSize.smajAxis;em.radio;stat.fit
#pragma db not_null
float maj_axis;

/// @brief FWHM minor axis before deconvolution (arcsec)
/// UCD: phys.angSize.sminAxis;em.radio;stat.fit
#pragma db not_null
float min_axis;

/// @brief Position angle before deconvolution (deg)
/// UCD: phys.angSize;pos.posAng;em.radio;stat.fit
#pragma db not_null
float pos_ang;

/// @brief Error in major axis before deconvolution (arcsec)
/// UCD: stat.error;phys.angSize.smajAxis;em.radio
#pragma db not_null
float maj_axis_err;

/// @brief Error in minor axis before deconvolution (arcsec)
/// UCD: stat.error;phys.angSize.sminAxis;em.radio
#pragma db not_null
float min_axis_err;

/// @brief Error in position angle before deconvolution (deg)
/// UCD: stat.error;phys.angSize;pos.posAng;em.radio
#pragma db not_null
float pos_ang_err;

/// @brief FWHM major axis after deconvolution (arcsec)
/// UCD: phys.angSize.smajAxis;em.radio;askap:meta.deconvolved
#pragma db not_null
float maj_axis_deconv;

/// @brief FWHM minor axis after deconvolution (arcsec)
/// UCD: phys.angSize.sminAxis;em.radio;askap:meta.deconvolved
#pragma db not_null
float min_axis_deconv;

/// @brief Position angle after deconvolution (deg)
/// UCD: phys.angSize;pos.posAng;em.radio;askap:meta.deconvolved
#pragma db not_null
float pos_ang_deconv;

/// @brief Chi-squared value of Gaussian fit
/// UCD: stat.fit.chi2
#pragma db not_null
float chi_squared_fit;

/// @brief RMS residual of Gaussian fit (mJy/beam)
/// UCD: stat.stdev;stat.fit
#pragma db not_null
float rms_fit_Gauss;

/// @brief Spectral index (First Taylor term)
/// UCD: spect.index;em.radio
#pragma db not_null
float spectral_index;

/// @brief Spectral curvature (Second Taylor term)
/// UCD: askap:spect.curvature;em.radio
#pragma db not_null
float spectral_curvature;

/// @brief rms noise level in image (mJy/beam)
/// UCD: stat.stdev;phot.flux.density
#pragma db not_null
float rms_image;

/// @brief Source has siblings
/// UCD: meta.code
#pragma db not_null
bool has_siblings;

/// @brief Component parameters are initial estimate, not from fit
/// UCD: meta.code
#pragma db not_null
bool fit_is_estimate;

