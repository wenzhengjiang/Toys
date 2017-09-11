module Main (main) where

import Test.Hspec

import Globber

main :: IO ()
main = hspec $ describe "Testing Globber" $ do

    describe "empty pattern" $ do
      it "matches empty string" $
        matchGlob "" "" `shouldBe` True
      it "shouldn't match non-empty string" $
        matchGlob "" "string" `shouldBe` False

