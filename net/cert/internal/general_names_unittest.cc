
TEST_P(ParseNameConstraints, GeneralNamesCreateFailsOnEmptySubjectAltName) {
  std::string invalid_san_der;
  ASSERT_TRUE(
      LoadTestSubjectAltNameData("san-invalid-empty.pem", &invalid_san_der));
  CertErrors errors;
  EXPECT_FALSE(GeneralNames::Create(der::Input(&invalid_san_der), &errors));
}

TEST_P(ParseNameConstraints,
       GeneralNamesCreateFailsOnInvalidIpInSubjectAltName) {
  std::string invalid_san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-invalid-ipaddress.pem",
                                         &invalid_san_der));
  CertErrors errors;
  EXPECT_FALSE(GeneralNames::Create(der::Input(&invalid_san_der), &errors));
}
