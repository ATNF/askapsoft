package askap.services.caldataservice;

import java.io.Serializable;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.logging.Level;
import java.util.logging.Logger;

import askap.interfaces.caldataservice.AlreadyExists;
import askap.interfaces.caldataservice.UnknownSolutionIdException;
import askap.interfaces.calparams.TimeTaggedBandpassSolution;
import askap.interfaces.calparams.TimeTaggedGainSolution;
import askap.interfaces.calparams.TimeTaggedLeakageSolution;

/**
 * Implements a storage for calibration solutions. Uses a MySQL database as an
 * index to binary file based storage of solution objects.
 *  
 * Copyright 2017, CSIRO Australia All rights reserved.
 * 
 */
public class MySQLStorage
{
  /** Logger */
  protected static final Logger LOG = Logger.getLogger(CdsServerI.class.getName());
  
  /** MySQL server connection */
  private Connection db ;
  
  /** Binary object storage object */
  private MatrixStorage bin ;

  /**
   * A constructor
   * @param driverName - name of driver class to use
   * @param dbConn - database URL
   * @param user - user name or null for unprotected connection
   * @param pass - password or null for unprotected connection
   * @param matrixStorageDir - path to directory to use for storage of binary objects
   * 
   */
  public MySQLStorage( String dbConn, String driverName, String user, String pass,
                                        String matrixStorageDir ) throws Exception
  {
    
    Class.forName( driverName );
    db = DriverManager.getConnection( dbConn, user, pass );
    db.setAutoCommit( true );
    try ( Statement statement = db.createStatement() )
    {
      statement.execute( SQL_SOLUTION ) ;
      statement.execute( SQL_IDGENERATOR ) ;
      try { statement.execute( SQL_INIT ) ; } catch( Exception e ) {}
      try { statement.execute( SQL_CREATE_INDEX1 ) ; } catch( Exception e ) {}
      try { statement.execute( SQL_CREATE_INDEX2 ) ; } catch( Exception e ) {}
    }
    bin = new MatrixStorage( matrixStorageDir, false ) ;
  }
  
  
  public void close() throws SQLException
  {
    if ( bin != null ) bin.close() ; 
    if ( db != null ) db.close() ; 
  }
  
  
  /**
   * Removes all records from the database
   * 
   * @throws SQLException
   */
  public void reset() throws SQLException
  {
    try ( Statement statement = db.createStatement() )
    {
      statement.execute( SQL_DELETE_SOLUTION ) ;
      statement.execute( SQL_DELETE_IDGENERATOR ) ;
    }
  }
  
  /**
   * Get the new solution ID
   *
   * Creates a new database entry for calibration solution(s) and returns unique id
   * which can be used together with add or update methods
   * @return the unique ID of the new entry to be populated later
   * @throws RuntimeException if there were database problems 
   */
  public long newSolutionID()
  {
    String sql = "UPDATE idgenerator SET curv = curv + 1 WHERE name = 'solution' " ;
    try ( PreparedStatement st = db.prepareStatement( sql ) )
    {
      long id = readLong( "SELECT curv FROM idgenerator WHERE name = 'solution' " ) ;
      st.execute() ;
      return id ;
    } catch ( Exception e )
    {
      LOG.log( Level.SEVERE, "Exception in newSolutionID: ", e );
      throw new RuntimeException( e ) ;
    }
  }
  
  
  /**
   * Add a new time tagged gains solution to the calibration
   * data service.
   * 
   * @param id entry to add the solution to
   * @param solution  new time tagged gains solution.
   * @note An exception is thrown if gain solution has already been added for this id
   */
  void addGainsSolution( long id, TimeTaggedGainSolution solution )
      throws AlreadyExists
  {
    double time = solution.timestamp ;
    addSolution( solution, id, time, 'g' ) ;
  }
  
  /**
   * Add a new time tagged bandpass solution to the calibration
   * data service.
   * 
   * @param id entry to add the solution to
   * @param solution  new time tagged bandpass solution.
   * @note An exception is thrown if bandpass solution has already been added for this id
   */
  void addBandpassSolution( long id, TimeTaggedBandpassSolution solution )
      throws AlreadyExists
  {
    double time = solution.timestamp ;
    addSolution( solution, id, time, 'b' ) ;    
  }
  
  /**
   * Add a new time tagged leakage solution to the calibration
   * data service.
   * 
   * @param id entry to add the solution to
   * @param solution  new time tagged leakage solution.
   * @note An exception is thrown if leakage solution has already been added for this id
   */
  void addLeakageSolution( long id, TimeTaggedLeakageSolution solution )
      throws AlreadyExists
  {
    double time = solution.timestamp ;
    addSolution( solution, id, time, 'l' ) ;        
  }


  // the following methods treat the given solution as an update relative to the most recent entry
  // which has a solution of the appropriate type. We can probably skip implementation of these
  // methods for version one, but this is what the calibration pipeline will be using.

  // one complication is that solutions may not always contain same antennas/beams, but it requires
  // more thought

  /**
   * Merge in a new time tagged gains solution with the latest gains and store it
   * in the calibration data service. If no previous solutions are present, store as is.
   * 
   * @param id entry to attach the resulting solution to
   * @param solution  new time tagged gains solution treated as an update
   * @note An exception is thrown if gain solution has already been added for this id
   */
  void adjustGains( long id, TimeTaggedGainSolution solution )
      throws AlreadyExists
  {
    throw new RuntimeException( "Not implemented" ) ;
  }

  /**
   * Merge in a new time tagged bandpass solution with the latest bandpass and store the 
   * result in the calibration data service. If no previous solutions are present, store as is.
   * 
   * @param id entry to attach the resulting solution to
   * @param solution  new time tagged bandpass solution.
   * @note An exception is thrown if bandpass solution has already been added for this id
   */
  void adjustBandpass( long id, TimeTaggedBandpassSolution solution )
      throws AlreadyExists
  {
    throw new RuntimeException( "Not implemented" ) ;    
  }
  
  /**
   * Merge in a new time tagged leakage solution with the latest leakage solution and store 
   * the result to the calibration data service (as a new solution)
   * 
   * @param id entry to add the solution to
   * @param solution  new time tagged leakage solution.
   * @note An exception is thrown if leakage solution has already been added for this id
   */
  void adjustLeakages( long id, TimeTaggedLeakageSolution solution )
      throws AlreadyExists
  {
    throw new RuntimeException( "Not implemented" ) ;        
  }

  // methods to check what kind of calibration information is attached to the given id

  /**
   * Check that the gain solution is present.
   * @param id entry to check
   * @return true, if the given id has a gain solution.
   */
  boolean hasGainSolution( long id )
  {
    return hasSolution( id, 'g' ) ; 
  }

  
  /**
   * Check that the leakage solution is present.
   * @param id entry to check
   * Obtain the ID of the current/optimum leakage solution.
   * @return true, if the given id has a leakage solution.
   */
  boolean hasLeakageSolution( long id )
  {
    return hasSolution( id, 'l' ) ;     
  }

  
  /**
   * Check that the bandpass solution is present.
   * @param id entry to check
   * @return true, if the given id has a bandpass solution.
   */
  boolean hasBandpassSolution( long id )
  {
    return hasSolution( id, 'b' ) ;     
  }
  
  
  /**
   * Obtain the most recent solution ID in the database
   * @note Solution ID corresponds to one entry in the database resembling a row
   * in the calibration table. It may contain gain, bandpass and leakage solutions
   * in any combinations (i.e. just gains, gains+bandpasses, all three, etc)
   * 
   * exception is thrown if the database is empty or any other fatal error
   * @return ID of the last entry
   */
  long getLatestSolutionID()
      throws UnknownSolutionIdException
  {
    long id = -1 ;
    try 
    {
      id = readLong( "SELECT curv FROM idgenerator WHERE name='solution'" ) - 1 ;
    }
    catch( Exception e )
    {
      throw new RuntimeException( e ) ;
    }
    if ( id < 0 )
      throw new UnknownSolutionIdException() ;
    return id ;
  }

  // the following two methods are intended for monitoring/visualisation
  // they are not used by ingest/calibration pipeline and can be further discussed

  /**
   * Obtain smallest solution ID corresponding to the time >= the given timestamp
   * @param timestamp absolute time given as MJD in the UTC frame (same as timestamp
   *                  in solutions - can be compared directly)
   * @return solution ID 
   * @note gain, bandpass and leakage solutions corresponding to one solution ID
   *       can have different timestamps. Use the greatest for comparison.
   * if all the timestamps in the stored solutions are less than the given timestamp,
   * this method is equivalent to getLatestSolutionID(). 
   */
  long getUpperBoundID( double timestamp )
              throws UnknownSolutionIdException
  {
    long id = -1 ;
    try 
      {
         id = readLong( "SELECT min(id) FROM solution WHERE time = (SELECT min(time) FROM solution WHERE time >= "
                        + timestamp + ") LIMIT 1" ) ;
      }
      catch( Exception e )
      {
        throw new RuntimeException( e ) ;
      }
    if ( id < 0 )
                throw new UnknownSolutionIdException() ;
    return id ;
  }

  /**
   * Obtain largest solution ID corresponding to the time <= the given timestamp
   * @param timestamp absolute time given as MJD in the UTC frame (same as timestamp
   *                  in solutions - can be compared directly)
   * @return solution ID 
   * @note gain, bandpass and leakage solutions corresponding to one solution ID
   *       can have different timestamps. Use the smallest for comparison.
   * if all the timestamps in the stored solutions are greater than the given timestamp,
   * this method should return zero. 
   */
  long getLowerBoundID( double timestamp )
      throws UnknownSolutionIdException
  {
    long id = -1 ;
    try 
      {
         
         id = readLong( "SELECT max(id) FROM solution WHERE time = (SELECT max(time) FROM solution WHERE time <= "
                        + timestamp + ") LIMIT 1" ) ;
      }
      catch( Exception e )
      {
        throw new RuntimeException( e ) ;
      }
    if ( id < 0 )
                throw new UnknownSolutionIdException() ;
    return id ;  
  }

  // access methods

  /**
   * Get a gain solution.
   * @param id    id of the gain solution to obtain.
   * @return the gain solution.
   *
   * @throws UnknownSolutionIdException   the id parameter does not refer to
   *                                      a known solution.
   */
  TimeTaggedGainSolution getGainSolution( long id )
      throws UnknownSolutionIdException
  {
     return (TimeTaggedGainSolution)getSolution( id, 'g' ) ;
  }

  /**
   * Get a leakage solution.
   * @param id    id of the leakage solution to obtain.
   * @return the leakage solution.
   *
   * @throws UnknownSolutionIdException   the id parameter does not refer to
   *                                      a known solution.
   */
  TimeTaggedLeakageSolution getLeakageSolution( long id )
      throws UnknownSolutionIdException
  {
    return (TimeTaggedLeakageSolution)getSolution( id, 'l' ) ;
  }

  /**
   * Get a bandpass solution.
   * @param id    id of the bandpass solution to obtain.
   * @return the bandpass solution.
   *
   * @throws UnknownSolutionIdException   the id parameter does not refer to
   *                                      a known solution.
   */
  TimeTaggedBandpassSolution getBandpassSolution( long id )
      throws UnknownSolutionIdException
  {
    return (TimeTaggedBandpassSolution)getSolution( id, 'b' ) ;
  }
  


  /**
   * Saves a serializable solution to matrix storage and creates a db record 
   * with its address and parameters
   * @param id
   * @param time
   * @param c
   */
  private void addSolution( Serializable solution, long id, double time, char type )
  throws AlreadyExists
  {
    if ( hasSolution( id, type ) )
        throw new AlreadyExists( "A solution of type " + type + " with id " + id + " already exists." ) ;
    
    String sql = "INSERT INTO solution ( id, type, time, file, addr, size ) "
                            + " VALUES ( ?, ?, ?, ?, ?, ? )";
    long[] arr = null ;
    try ( PreparedStatement st = db.prepareStatement( sql ) )
    {
      arr = bin.writeObject( solution ) ;
      st.setLong( 1, id ) ;
      st.setString( 2, "" + type ) ;
      st.setDouble( 3, time ) ;
      st.setInt( 4, (int)arr[ 0 ] ) ;
      st.setLong( 5, (long)arr[ 1 ]  ) ;
      st.setInt( 6, (int)arr[ 2 ]  ) ;
      st.execute() ;
    }
    catch( Exception e )
    { // If already wrote the object to the file, roll back the file
      if ( arr != null )
        try { bin.rollBack( arr[ 1 ] ) ; } catch( Exception e1 ) {} 
      throw new RuntimeException( e ) ;
    }

  }

  
  /**
   * Checks if this solution exists
   * @param id - solution id
   * @param type - solution type: 'g', 'l', or 'b' 
   * @return
   */
  private boolean hasSolution( long id, char type )
  {
    long existing = -1 ;
    
    try
    {
      existing = readLong( "SELECT id FROM solution WHERE id=" + id + " and type ='" + type + "'" ) ;
    } catch ( Exception e1 )
    {
      throw new RuntimeException( e1 ) ;
    }
    
    return existing > 0 ;
  }


  /**
   * Reads solution with required id and type
   * @param id - solyution id
   * @param type - solution type: 'g', 'l', or 'b' 
   * @return
   */
  public Object getSolution( long id, char type ) throws UnknownSolutionIdException
  {
    String sql = "SELECT file, size, addr FROM solution WHERE id = " + id + " AND type = '" + type + "'" ;
    Object solution = null ;
    try
    ( 
      PreparedStatement st = db.prepareStatement( sql ) ;
      ResultSet rs = st.executeQuery() ;
    )
    {
      if ( rs.first() )
      {
         int file = rs.getInt( 1 ) ;
         int size = rs.getInt( 2 ) ;
         long addr = rs.getLong( 3 ) ; 
         solution = bin.readObject( file, addr, size ) ;      
         return solution ;
      }
    }
    catch( Exception e )
    {
      throw new RuntimeException( e ) ;
    }
    throw new UnknownSolutionIdException() ;
  }


  /**
   * Read a long value
   * 
   * @param sql
   *          String sql statement that is expected to produce a result set
   * @return long value
   */
  public long readLong( String sql ) throws Exception
  {
    long value = -1;
    try
    ( 
      PreparedStatement st = db.prepareStatement( sql ) ;
      ResultSet rs = st.executeQuery() ;
    )
    {
      if ( rs.first() )
      {
         value = rs.getLong( 1 ) ; 
      }
    }
    catch( Exception e )
    {
      throw new RuntimeException( e ) ;
    }
    return value;
  }

  /** Create main table */
  static private final String SQL_SOLUTION = "CREATE TABLE IF NOT EXISTS solution ( " +
      " id             BIGINT(20), " + // solution id
      " type           CHAR(1),  " + // b - bandpass, g - gain
      " time           DOUBLE NOT NULL, " + // configuration creation time
      " size           INT NOT NULL, " + // size of the stored object
      " file           INT NOT NULL, " + // number of the file where the object is stored
      " addr           BIGINT NOT NULL ) " ; // address in the file where the object is stored
  
  /** Create id generator table */
  static private final String SQL_IDGENERATOR = "CREATE TABLE IF NOT EXISTS idgenerator ( " +
      "name  varchar(100) NOT NULL, " +
      "curv  bigint(20) unsigned DEFAULT 1, " + 
      "PRIMARY KEY ( name ) ) " ; 

  /** Create index on id and type */
  static private final String SQL_CREATE_INDEX1 = "CREATE INDEX idxtype ON solution( id, type )" ;
  
  /** Create index on id and time */
  static private final String SQL_CREATE_INDEX2 = "CREATE INDEX idxtime ON solution( id, time )" ;
    
  /** Delete all from the main table */
  static private final String SQL_DELETE_SOLUTION = "DELETE FROM solution" ;
  
  /** Delete all from the id generator table */
  static private final String SQL_DELETE_IDGENERATOR = "DELETE FROM idgenerator" ;
  
  /** Init the id generator */
  static private final String SQL_INIT = "INSERT INTO idgenerator VALUES('solution', 1)" ;

}
