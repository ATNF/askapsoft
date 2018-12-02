/// @brief Component identifier
/// UCD: meta.id;meta.main
#pragma db not_null
std::string component_id;

/// @brief Band-median value for Stokes I spectrum (mJy/beam)
/// UCD: phot.flux.density;em.radio
#pragma db not_null
double flux_I_median;

/// @brief Band-median value for Stokes Q spectrum (mJy/beam)
/// UCD: phot.flux.density;em.radio;askap:phys.polarization.stokes.Q
#pragma db not_null
double flux_Q_median;

/// @brief Band-median value for Stokes U spectrum (mJy/beam)
/// UCD: phot.flux.density;em.radio;askap:phys.polarization.stokes.U
#pragma db not_null
double flux_U_median;

/// @brief Band-median value for Stokes V spectrum (mJy/beam)
/// UCD: phot.flux.density;em.radio;askap:phys.polarization.stokes.V
#pragma db not_null
double flux_V_median;

/// @brief Band-median sensitivity for Stokes I spectrum (mJy/beam)
/// UCD: stat.stdev;phot.flux.density
#pragma db not_null
double rms_I;

/// @brief Band-median sensitivity for Stokes Q spectrum (mJy/beam)
/// UCD: stat.stdev;phot.flux.density;askap:phys.polarization.stokes.Q
#pragma db not_null
double rms_Q;

/// @brief Band-median sensitivity for Stokes U spectrum (mJy/beam)
/// UCD: stat.stdev;phot.flux.density;askap:phys.polarization.stokes.U
#pragma db not_null
double rms_U;

/// @brief Band-median sensitivity for Stokes V spectrum (mJy/beam)
/// UCD: stat.stdev;phot.flux.density;askap:phys.polarization.stokes.V
#pragma db not_null
double rms_V;

/// @brief First order coefficient for polynomial fit to Stokes I spectrum
/// UCD: stat.fit.param;spect.continuum
#pragma db not_null
double co_1;

/// @brief Second order coefficient for polynomial fit to Stokes I spectrum
/// UCD: stat.fit.param;spect.continuum
#pragma db not_null
double co_2;

/// @brief Third order coefficient for polynomial fit to Stokes I spectrum
/// UCD: stat.fit.param;spect.continuum
#pragma db not_null
double co_3;

/// @brief Fourth order coefficient for polynomial fit to Stokes I spectrum
/// UCD: stat.fit.param;spect.continuum
#pragma db not_null
double co_4;

/// @brief Fifth order coefficient for polynomial fit to Stokes I spectrum
/// UCD: stat.fit.param;spect.continuum
#pragma db not_null
double co_5;

/// @brief Reference wavelength squared (m^2)
/// UCD: askap:em.wl.squared
#pragma db not_null
double lambda_ref_sq;

/// @brief Full-width at half maximum of the rotation measure spread function (rad/m^2)
/// UCD: phys.polarization.rotMeasure;askap:phys.polarization.rmsfWidth
#pragma db not_null
double rmsf_fwhm;

/// @brief Peak polarised intensity in the Faraday Dispersion Function (mJy/beam)
/// UCD: phot.flux.density;phys.polarization.rotMeasure;stat.max
#pragma db not_null
double pol_peak;

/// @brief Effective peak polarised intensity after correction for bias (mJy/beam)
/// UCD: phot.flux.density;phys.polarization.rotMeasure;stat.max;askap:meta.corrected
#pragma db not_null
double pol_peak_debias;

/// @brief Uncertainty in pol_peak (mJy/beam)
/// UCD: stat.error;phot.flux.density;phys.polarization.rotMeasure;stat.max
#pragma db not_null
double pol_peak_err;

/// @brief Peak polarised intensity from a three-point parabolic fit (mJy/beam)
/// UCD: phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit
#pragma db not_null
double pol_peak_fit;

/// @brief Peak polarised intensity, corrected for bias, from a three-point parabolic fit (mJy/beam)
/// UCD: phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit;askap:meta.corrected
#pragma db not_null
double pol_peak_fit_debias;

/// @brief Uncertainty in pol_peak_fit (mJy/beam)
/// UCD: stat.error;phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit
#pragma db not_null
double pol_peak_fit_err;

/// @brief Signal-to-noise ratio of the peak polarisation
/// UCD: stat.snr;phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit
#pragma db not_null
double pol_peak_fit_snr;

/// @brief Uncertainty in pol_peak_fit_snr
/// UCD: stat.error;stat.snr;phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit
#pragma db not_null
double pol_peak_fit_snr_err;

/// @brief Faraday Depth from the channel with the peak of the Faraday Dispersion Function (rad/m^2)
/// UCD: phys.polarization.rotMeasure
#pragma db not_null
double fd_peak;

/// @brief Uncertainty in far_depth_peak (rad/m^2)
/// UCD: stat.error;phys.polarization.rotMeasure
#pragma db not_null
double fd_peak_err;

/// @brief Faraday Depth from fit to peak in Faraday Dispersion Function (rad/m^2)
/// UCD: phys.polarization.rotMeasure;stat.fit
#pragma db not_null
double fd_peak_fit;

/// @brief uncertainty in fd_peak_fit (rad/m^2)
/// UCD: stat.error;phys.polarization.rotMeasure;stat.fit
#pragma db not_null
double fd_peak_fit_err;

/// @brief Polarisation angle at the reference wavelength (deg)
/// UCD: askap:phys.polarization.angle
#pragma db not_null
double pol_ang_ref;

/// @brief Uncertainty in pol_ang_ref (deg)
/// UCD: stat.error;askap:phys.polarization.angle
#pragma db not_null
double pol_ang_ref_err;

/// @brief Polarisation angle de-rotated to zero wavelength (deg)
/// UCD: askap:phys.polarization.angle;askap:meta.corrected
#pragma db not_null
double pol_ang_zero;

/// @brief Uncertainty in pol_ang_zero (deg)
/// UCD: stat.error;askap:phys.polarization.angle;askap:meta.corrected
#pragma db not_null
double pol_ang_zero_err;

/// @brief Fractional polarisation
/// UCD: phys.polarization
#pragma db not_null
double pol_frac;

/// @brief Uncertainty in fractional polarisation
/// UCD: stat.error;phys.polarization
#pragma db not_null
double pol_frac_err;

/// @brief Statistical measure of polarisation complexity
/// UCD: stat.value;phys.polarization
#pragma db not_null
double complex_1;

/// @brief Statistical measure of polarisation complexity after removal of a thin-screen model.
/// UCD: stat.value;phys.polarization
#pragma db not_null
double complex_2;

/// @brief True if pol_peak_fit is above a threshold value otherwise pol_peak_fit is an upper limit.
/// UCD: meta.code
#pragma db not_null
bool flag_p1;

/// @brief True if FDF peak is close to edge
/// UCD: meta.code
#pragma db not_null
bool flag_p2;

/// @brief placeholder flag
/// UCD: meta.code
#pragma db not_null
bool flag_p3;

/// @brief placeholder flag
/// UCD: meta.code
#pragma db not_null
bool flag_p4;

