-- MySQL dump 10.13  Distrib 8.0.30, for Win64 (x86_64)
--
-- Host: 121.169.251.200    Database: creverse
-- ------------------------------------------------------
-- Server version	8.0.30

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `1_serverip`
--

DROP TABLE IF EXISTS `1_serverip`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `1_serverip` (
  `index` int unsigned NOT NULL,
  `type` char(16) DEFAULT NULL COMMENT '서버 타입 .. PxDefine.h 의 ParseServerType 함수에서 문자열 파싱을 하고 있다.',
  `svrRegion` tinyint unsigned DEFAULT NULL COMMENT '서버 지역 ex) 아시아, 유럽, 북미 등',
  `svrGroup` tinyint unsigned DEFAULT NULL COMMENT '서버 지역 그룹\n지역에 Main 서버가 늘어나면 그룹이 생성된다.',
  `svrIndex` tinyint unsigned DEFAULT '0' COMMENT '서버 그룹별 index',
  `svrName` char(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL COMMENT '식별용 서버 이름.. 큰 의미 없다.',
  `svrStatus` tinyint unsigned DEFAULT '0' COMMENT '0 - Normal\\n1 - NormalButLocked - 캐릭터 생성 불가\\n2 - Maintenance\\n3 - Off\\n',
  `maintenanceEndTime` char(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL COMMENT '서버 점검시 서버 점검 끝나는 시간을 적어놓고 클라이언트에 전파하는 용도.',
  `privateIP` char(20) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL COMMENT 'AWS 인스턴스 생성시 부여되는 내부 연결용 IP (Private IP)\\n서버간 연결에 사용한다.',
  `publicIP` char(20) DEFAULT NULL COMMENT 'AWS 인스턴스 생성시 부여되는 외부 연결용 IP (Public IP)\\n클라이언트 - 서버 연결에 사용한다.',
  `portClientTCP` smallint unsigned DEFAULT NULL COMMENT '클라이언트에게 개방하는 포트 번호',
  `portClientUDP` smallint unsigned DEFAULT NULL COMMENT '클라이언트에게 개방하는 UDP 포트 번호 (현재는 안쓰임)',
  `portServer` smallint unsigned DEFAULT NULL COMMENT '서버간 연결에 사용하는 내부 포트',
  `endMarker` smallint unsigned DEFAULT NULL COMMENT '클라이언트가 서버에 패킷 전송시 끝에 붙이는 16Bit 마커\n클라 패킷 검증에 쓰인다.',
  PRIMARY KEY (`index`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='서버간 연결 혹은 클라이언트 연결 정보';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `1_serverip`
--

LOCK TABLES `1_serverip` WRITE;
/*!40000 ALTER TABLE `1_serverip` DISABLE KEYS */;
INSERT INTO `1_serverip` VALUES (1,'GLOBAL',0,0,0,NULL,0,NULL,'172.16.32.139','172.16.32.139',52310,0,53310,21411),(2,'LOGIN',0,0,0,'',0,NULL,'172.16.32.139','172.16.32.139',52320,0,53320,53874),(3,'MAIN',0,0,0,'Korea1',0,NULL,'172.16.32.139','172.16.32.139',52410,0,53410,53794),(4,'MAIN',0,1,0,'Korea2',0,NULL,'172.16.32.139','172.16.32.139',52411,0,53411,38648);
/*!40000 ALTER TABLE `1_serverip` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `1_serverip_gs63vr`
--

DROP TABLE IF EXISTS `1_serverip_gs63vr`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `1_serverip_gs63vr` (
  `index` int unsigned NOT NULL,
  `type` char(16) DEFAULT NULL COMMENT '서버 타입 .. PxDefine.h 의 ParseServerType 함수에서 문자열 파싱을 하고 있다.',
  `svrRegion` tinyint unsigned DEFAULT NULL COMMENT '서버 지역 ex) 아시아, 유럽, 북미 등',
  `svrGroup` tinyint unsigned DEFAULT NULL COMMENT '서버 지역 그룹\n지역에 Main 서버가 늘어나면 그룹이 생성된다.',
  `svrIndex` tinyint unsigned DEFAULT '0' COMMENT '서버 그룹별 index',
  `svrName` char(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL COMMENT '식별용 서버 이름.. 큰 의미 없다.',
  `svrStatus` tinyint unsigned DEFAULT '0' COMMENT '0 - Normal\\n1 - NormalButLocked - 캐릭터 생성 불가\\n2 - Maintenance\\n3 - Off\\n',
  `maintenanceEndTime` char(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL COMMENT '서버 점검시 서버 점검 끝나는 시간을 적어놓고 클라이언트에 전파하는 용도.',
  `privateIP` char(20) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL COMMENT 'AWS 인스턴스 생성시 부여되는 내부 연결용 IP (Private IP)\\n서버간 연결에 사용한다.',
  `publicIP` char(20) DEFAULT NULL COMMENT 'AWS 인스턴스 생성시 부여되는 외부 연결용 IP (Public IP)\\n클라이언트 - 서버 연결에 사용한다.',
  `portClientTCP` smallint unsigned DEFAULT NULL COMMENT '클라이언트에게 개방하는 포트 번호',
  `portClientUDP` smallint unsigned DEFAULT NULL COMMENT '클라이언트에게 개방하는 UDP 포트 번호 (현재는 안쓰임)',
  `portServer` smallint unsigned DEFAULT NULL COMMENT '서버간 연결에 사용하는 내부 포트',
  `endMarker` smallint unsigned DEFAULT NULL COMMENT '클라이언트가 서버에 패킷 전송시 끝에 붙이는 16Bit 마커\n클라 패킷 검증에 쓰인다.',
  PRIMARY KEY (`index`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='서버간 연결 혹은 클라이언트 연결 정보';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `1_serverip_gs63vr`
--

LOCK TABLES `1_serverip_gs63vr` WRITE;
/*!40000 ALTER TABLE `1_serverip_gs63vr` DISABLE KEYS */;
INSERT INTO `1_serverip_gs63vr` VALUES (1,'GLOBAL',0,0,0,NULL,0,NULL,'172.16.32.120','172.16.32.120',52310,0,53310,21411),(2,'LOGIN',0,0,0,'',0,NULL,'172.16.32.120','172.16.32.120',52320,0,53320,53874),(3,'MAIN',0,0,0,'Korea1',0,NULL,'172.16.32.120','172.16.32.120',52410,0,53410,53794),(4,'MAIN',0,1,0,'Korea2',0,NULL,'172.16.32.120','172.16.32.120',52411,0,53411,38648),(5,'ZONE',0,0,0,'Korea1-Z1',0,NULL,'172.16.32.120','172.16.32.120',52520,0,53520,14526),(6,'ZONE',0,0,1,'Korea1-Z2',0,NULL,'172.16.32.120','172.16.32.120',52521,0,53521,19234);
/*!40000 ALTER TABLE `1_serverip_gs63vr` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `1_serverip_svr`
--

DROP TABLE IF EXISTS `1_serverip_svr`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `1_serverip_svr` (
  `index` int unsigned NOT NULL,
  `type` char(16) DEFAULT NULL COMMENT '서버 타입 .. PxDefine.h 의 ParseServerType 함수에서 문자열 파싱을 하고 있다.',
  `svrRegion` tinyint unsigned DEFAULT NULL COMMENT '서버 지역 ex) 아시아, 유럽, 북미 등',
  `svrGroup` tinyint unsigned DEFAULT NULL COMMENT '서버 지역 그룹\n지역에 Main 서버가 늘어나면 그룹이 생성된다.',
  `svrIndex` tinyint unsigned DEFAULT '0' COMMENT '서버 그룹별 index',
  `svrName` char(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL COMMENT '식별용 서버 이름.. 큰 의미 없다.',
  `svrStatus` tinyint unsigned DEFAULT '0' COMMENT '0 - Normal\\n1 - NormalButLocked - 캐릭터 생성 불가\\n2 - Maintenance\\n3 - Off\\n',
  `maintenanceEndTime` char(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL COMMENT '서버 점검시 서버 점검 끝나는 시간을 적어놓고 클라이언트에 전파하는 용도.',
  `privateIP` char(20) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL COMMENT 'AWS 인스턴스 생성시 부여되는 내부 연결용 IP (Private IP)\\n서버간 연결에 사용한다.',
  `publicIP` char(20) DEFAULT NULL COMMENT 'AWS 인스턴스 생성시 부여되는 외부 연결용 IP (Public IP)\\n클라이언트 - 서버 연결에 사용한다.',
  `portClientTCP` smallint unsigned DEFAULT NULL COMMENT '클라이언트에게 개방하는 포트 번호',
  `portClientUDP` smallint unsigned DEFAULT NULL COMMENT '클라이언트에게 개방하는 UDP 포트 번호 (현재는 안쓰임)',
  `portServer` smallint unsigned DEFAULT NULL COMMENT '서버간 연결에 사용하는 내부 포트',
  `endMarker` smallint unsigned DEFAULT NULL COMMENT '클라이언트가 서버에 패킷 전송시 끝에 붙이는 16Bit 마커\n클라 패킷 검증에 쓰인다.',
  PRIMARY KEY (`index`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='서버간 연결 혹은 클라이언트 연결 정보';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `1_serverip_svr`
--

LOCK TABLES `1_serverip_svr` WRITE;
/*!40000 ALTER TABLE `1_serverip_svr` DISABLE KEYS */;
INSERT INTO `1_serverip_svr` VALUES (1,'GLOBAL',0,0,0,NULL,0,NULL,'172.16.32.148','172.16.32.148',52310,0,53310,21411),(2,'LOGIN',0,0,0,'',0,NULL,'172.16.32.148','172.16.32.148',52320,0,53320,53874),(3,'MAIN',0,0,0,'Korea1',0,NULL,'172.16.32.148','172.16.32.148',52410,0,53410,53794),(4,'MAIN',0,1,0,'Korea2',0,NULL,'172.16.32.148','172.16.32.148',52411,0,53411,38648),(5,'ZONE',0,0,0,'Korea1-Z1',0,NULL,'172.16.32.148','172.16.32.148',52520,0,53520,14526),(6,'ZONE',0,0,1,'Korea1-Z2',0,NULL,'172.16.32.148','172.16.32.148',52521,0,53521,19234);
/*!40000 ALTER TABLE `1_serverip_svr` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `2_guidtimechunk`
--

DROP TABLE IF EXISTS `2_guidtimechunk`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `2_guidtimechunk` (
  `timeStamp` int unsigned NOT NULL,
  `chunkCount` smallint unsigned DEFAULT NULL,
  PRIMARY KEY (`timeStamp`),
  UNIQUE KEY `timeStamp_UNIQUE` (`timeStamp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `2_guidtimechunk`
--

LOCK TABLES `2_guidtimechunk` WRITE;
/*!40000 ALTER TABLE `2_guidtimechunk` DISABLE KEYS */;
/*!40000 ALTER TABLE `2_guidtimechunk` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `errorlog`
--

DROP TABLE IF EXISTS `errorlog`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `errorlog` (
  `index` int unsigned NOT NULL AUTO_INCREMENT,
  `type` tinyint unsigned DEFAULT NULL,
  `log` char(255) DEFAULT NULL,
  UNIQUE KEY `index_UNIQUE` (`index`)
) ENGINE=InnoDB AUTO_INCREMENT=245 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `errorlog`
--

LOCK TABLES `errorlog` WRITE;
/*!40000 ALTER TABLE `errorlog` DISABLE KEYS */;
/*!40000 ALTER TABLE `errorlog` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2024-01-31 10:50:38
