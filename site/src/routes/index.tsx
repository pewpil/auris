import Nav from "~/components/Nav";
import Hero from "~/components/Hero";
import HowItWorks from "~/components/HowItWorks";
import Features from "~/components/Features";
import TechStack from "~/components/TechStack";
import Roadmap from "~/components/Roadmap";
import Footer from "~/components/Footer";

export default function Home() {
  return (
    <>
      <Nav />
      <main>
        <Hero />
        <HowItWorks />
        <Features />
        <TechStack />
        <Roadmap />
      </main>
      <Footer />
    </>
  );
}
