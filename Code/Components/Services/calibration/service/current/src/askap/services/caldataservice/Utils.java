package askap.services.caldataservice;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.ObjectOutput;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;

import askap.interfaces.FloatComplex;
import askap.interfaces.calparams.JonesDTerm;
import askap.interfaces.calparams.JonesIndex;
import askap.interfaces.calparams.JonesJTerm;
import askap.interfaces.calparams.TimeTaggedBandpassSolution;
import askap.interfaces.calparams.TimeTaggedGainSolution;
import askap.interfaces.calparams.TimeTaggedLeakageSolution;

/**
 * A set of shared static methods
 * 
 * Copyright 2017, CSIRO Australia
 * All rights reserved.
 * 
 */
public class Utils
{
  
  public static void main( String[] args ) throws Exception
  {
    deleteDirectory( "D:/temp/test" ) ;
  }
    
  /**
   * @param tag - integer value to base numbers in the solution on 
   * @return mocked solution
   */
  public static TimeTaggedLeakageSolution makeLeakageSolution( int tag )
  {
    TimeTaggedLeakageSolution r = new TimeTaggedLeakageSolution() ;
    r.timestamp = tag ;
    r.solutionMap = new HashMap<JonesIndex, JonesDTerm>() ;
    
    for ( short i = 0 ; i < 10 * tag ; i++ )
    {
      JonesIndex index = new JonesIndex() ;
      index.antennaID = i ;
      index.beamID = i ;
      JonesDTerm term = new JonesDTerm() ;
      term.d12 = new FloatComplex() ;
      term.d12.imag = i ;
      term.d12.real = i ;

      term.d21 = new FloatComplex() ;
      term.d21.imag = i ;
      term.d21.real = i ;
      r.solutionMap.put( index, term ) ;
    }
    return r ;
  }

  /**
   * @param tag - integer value to base numbers in the solution on 
   * @return mocked solution
   */
  public static TimeTaggedGainSolution makeGainSolution( int tag )
  {
    TimeTaggedGainSolution r = new TimeTaggedGainSolution() ;
    r.timestamp = tag ;
    r.solutionMap = new HashMap<JonesIndex, JonesJTerm>() ;
    
    for ( short i = 0 ; i < 10 * tag ; i++ )
    {
      JonesIndex index = new JonesIndex() ;
      index.antennaID = i ;
      index.beamID = i ;
      JonesJTerm term = new JonesJTerm() ;
      term.g1 = new FloatComplex() ;
      term.g1.imag = i ;
      term.g1.real = i ;
      term.g1Valid = tag % ( i + 1 ) == 0 ;

      term.g2 = new FloatComplex() ;
      term.g2.imag = tag ;
      term.g2.real = tag ;
      term.g2Valid = tag % ( i + 1 ) != 0 ;
      r.solutionMap.put( index, term ) ;
    }
    return r ;
  }

  /**
   * @param tag - integer value to base numbers in the solution on 
   * @return mocked solution
   */
  public static TimeTaggedBandpassSolution makeBandpassSolution( int tag )
  {
    TimeTaggedBandpassSolution r = new TimeTaggedBandpassSolution() ;
    r.timestamp = tag ;
    r.solutionMap = new HashMap<JonesIndex, java.util.List<JonesJTerm>>() ;
    
    for ( short i = 0 ; i < 10 * tag ; i++ )
    {
      JonesIndex index = new JonesIndex() ;
      index.antennaID = i ;
      index.beamID = i ;
      
      ArrayList<JonesJTerm> list = new ArrayList<JonesJTerm>() ;
      for ( int j = 0 ; j < i ; j++ )
      {
        JonesJTerm term = new JonesJTerm() ;
        term.g1 = new FloatComplex() ;
        term.g1.imag = j ;
        term.g1.real = j ;
        term.g1Valid = tag % ( j + 1 ) == 0 ;

        term.g2 = new FloatComplex() ;
        term.g2.imag = tag ;
        term.g2.real = tag ;
        term.g2Valid = tag % ( j + 1 ) != 0 ;
        list.add( term ) ;
      }

      r.solutionMap.put( index, list ) ;
    }
    return r ;
  }
  
  
  public static boolean equal( Serializable s1, Serializable  s2 )
  {
    try
    {
      ByteArrayOutputStream bos1 = new ByteArrayOutputStream() ;
      ByteArrayOutputStream bos2 = new ByteArrayOutputStream() ;
      ObjectOutput out1 = new ObjectOutputStream( bos1 ) ;
      ObjectOutput out2 = new ObjectOutputStream( bos2 ) ;
      out1.writeObject( s1 ) ;
      out2.writeObject( s2 ) ;
      out1.flush() ;
      out2.flush() ;
      byte[] bytes1 = bos1.toByteArray() ;
      byte[] bytes2 = bos2.toByteArray() ;
      return Arrays.equals( bytes1, bytes2 ) ;
    }
    catch( Exception e ) 
    {
      return false ;
    }
  }
  
  
  public static boolean equalB( TimeTaggedBandpassSolution s1, TimeTaggedBandpassSolution s2 )
  {
    try
    {
      if ( s1 == null && s2 == null ) return true ;
      if ( s1 == null || s2 == null ) return false ;
      return s1.equals( s2 ) ;
    }
    catch( Exception e ) 
    {
      return false ;
    }
  }
  
  
  

  /**
   * Delete a directory, possibly, recursively
   * 
   * @param dir path to the directory
   * @throws Exception 
   */
  public static void deleteDirectory( String dir ) throws Exception
  {
    File dirFile = new File( dir ) ;
    File[] files = dirFile.listFiles() ;
    for ( File file : files )
    {
      String name = file.getName() ;
      if ( name.equals( "." ) || name.equals( ".." ) ) continue ;
      if ( file.isDirectory() )
         deleteDirectory( file.getAbsolutePath() ) ;
      else if ( !file.delete() ) 
             throw new Exception( "Cain't delete file " + file ) ; 
    }
    if ( !dirFile.delete() ) 
      throw new Exception( "Cain't delete file " + dirFile ) ; 

  }

  /**
   * 
   */
  public Utils()
  {
    // TODO Auto-generated constructor stub
  }

}
