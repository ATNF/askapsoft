package askap.services.caldataservice;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 * 
 * 
 * Copyright 2017, CSIRO Australia
 * All rights reserved.
 * 
 */
public class MatrixStorageTest
{

  String tempDir = null ;
  MatrixStorage s ;
  
  /**
   * @throws java.lang.Exception
   */
  @BeforeClass
  public static void setUpBeforeClass() throws Exception
  {}

  /**
   * @throws java.lang.Exception
   */
  @AfterClass
  public static void tearDownAfterClass() throws Exception
  {}

  /**
   * @throws java.lang.Exception
   */
  @Before
  public void setUp() throws Exception
  {
    Path temp = Files.createTempDirectory( "temp" ) ;
    tempDir = temp.toString() ;
  }

  /**
   * @throws java.lang.Exception
   */
  @After
  public void tearDown() throws Exception
  {
    if ( s != null ) try { s.close() ; } catch( Exception e ) {};
    File toDelete = new File( tempDir ) ;
    Utils.deleteDirectory( toDelete.getAbsolutePath() ) ;
  }

  @Test
  public void test() throws Exception
  {
    byte[] arr = { 1, 2, 3, 4, 5, 6, 7 } ;
    
    s = new MatrixStorage( tempDir, true ) ;
    MatrixStorage.setMaxFileSize( 20 ) ;
    
    // Writing 
    for ( int i = 0 ; i < 10 ; i++ )
      s.write( arr ) ;
    s.close() ;
    // More writing 
    s = new MatrixStorage( tempDir, false ) ;
    for ( int i = 0 ; i < 10 ; i++ )
      s.write( arr ) ;
      
    // Normal reading 
    byte[] arr2 = s.read( 1, 0, arr.length ) ;
    assert( arr2[ 0 ] == arr[ 0 ] && arr2[ 6 ] == arr[ 6 ] ) ;
    arr2 = s.read( 1, 7, arr.length ) ;
    assert( arr2[ 0 ] == arr[ 0 ] && arr2[ 6 ] == arr[ 6 ] ) ;
    // Gzip file #2 to check reading from gzipped file
    try // It does not exist unless manually compressed
    {
      arr2 = s.read( 2, 7, arr.length ) ;
      assert( arr2[ 0 ] == arr[ 0 ] && arr2[ 6 ] == arr[ 6 ] ) ;
    }
    catch( Exception e )
    {
      
    }
    
    
    // Reading from current file
    s.write( arr ) ;
    arr2 = s.read( 11, 0, arr.length ) ;
    assert( arr2[ 0 ] == arr[ 0 ] && arr2[ 6 ] == arr[ 6 ] ) ;
    // Writing after reading 
    s.write( arr ) ;
    arr2 = s.read( 11, 7, arr.length ) ;
    assert( arr2[ 0 ] == arr[ 0 ] && arr2[ 6 ] == arr[ 6 ] ) ;
    
    // Reading a wrong file
    try 
    {
      arr2 = s.read( 100, 7, arr.length ) ;
      assert( false ) ;
    }
    catch( Exception e )
    {
      assert( e.getMessage().contains( "File to read does not exist" ) ) ;
    }
    
    // Reading at wrong address 
    try 
    {
      arr2 = s.read( 1, 17, arr.length ) ;
      assert( false ) ;
    }
    catch( Exception e )
    {
      assert( e.getMessage().contains( "Could not read" ) ) ;
    } 
    
    // Batch reading - normal
    int addr[] = { 1, 0, 7, 1, 7, 7, 2, 0, 7, 2, 7, 7 } ;
    byte[][] batch = s.read( addr ) ;
    for ( int i = 0, j = 0 ; i < addr.length ; i += 3, j++ )
    {
      arr2 = batch[ j ] ;
      assert( arr2[ 0 ] == arr[ 0 ] && arr2[ 6 ] == arr[ 6 ] ) ;
    }
    
    // Batch reading of unordered addresses
    addr = new int[] { 1, 7, 7, 1, 0, 7, 3, 0, 7, 2, 7, 7 } ;
    batch = s.read( addr ) ;
    for ( int i = 0, j = 0 ; i < addr.length ; i += 3, j++ )
    {
      arr2 = batch[ j ] ;
      assert( arr2[ 0 ] == arr[ 0 ] && arr2[ 6 ] == arr[ 6 ] ) ;
    }
    
  }

}
